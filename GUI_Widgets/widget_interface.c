// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include "thread_pool.h"

#include <allegro5/allegro_font.h>
#include <allegro5/allegro_opengl.h>

#include <stdio.h>
#include <float.h>
#include <math.h>

// TODO: Remove
void stack_dump(lua_State* L)
{
    int top = lua_gettop(L);
    printf("Stack Dump (%d):\n", top);
    for (int i = 1; i <= top; i++) {
        printf("\t%d\t%s\t", i, luaL_typename(L, i));
        switch (lua_type(L, i)) {
        case LUA_TNUMBER:
            printf("\t%g\n", lua_tonumber(L, i));
            break;
        case LUA_TSTRING:
            printf("\t%s\n", lua_tostring(L, i));
            break;
        case LUA_TBOOLEAN:
            printf("\t%s\n", (lua_toboolean(L, i) ? "true" : "false"));
            break;
        case LUA_TNIL:
            printf("\t%s\n", "nil");
            break;
        default:
            printf("\t%p\n", lua_topointer(L, i));
            break;
        }
    }
}

extern void style_element_setup();
extern void style_element_predraw(const struct style_element* const);

extern double mouse_x;
extern double mouse_y;
extern double current_timestamp;
extern double delta_timestamp;
extern const ALLEGRO_FONT* debug_font;
extern ALLEGRO_EVENT current_event;
extern const ALLEGRO_TRANSFORM identity_transform;

// TODO: try to remove, still used in calling lua methods
extern lua_State* const main_lua_state;

#define FOR_CALLBACKS(DO) \
	DO(right_click) \
	DO(left_click) \
	DO(hover_start) \
	DO(hover_end) \
	DO(click_off) \
	DO(drag_start) \
	DO(drag_end_drop) \
	DO(drag_end_no_drop) \
	DO(drop_start) \
	DO(drop_end) \

struct widget
{
	struct widget_interface;
    struct widget_jump_table* jump_table;

    struct widget* next;
    struct widget* previous;

    struct
    {
#define DECLARE(method) int method;
        FOR_CALLBACKS(DECLARE)
    } lua;
};

#define call_lua(widget,method) if(widget->lua. ## method != LUA_REFNIL) \
    do{lua_rawgeti(main_lua_state, LUA_REGISTRYINDEX, widget->lua. ## method); lua_pcall(main_lua_state, 0, 0, 0); } while(0);

#define call_engine(widget,method) if(widget->jump_table-> ## method) \
    do{(widget)->jump_table-> ## method((struct widget_interface*) (widget));}while(0);

#define call(widget,method) do{call_engine(widget,method) call_lua(widget,method)}while(0);

#define call_va(widget,method,...) (widget)->jump_table->method((struct widget_interface*) (widget),__VA_ARGS__)

static enum {
    ENGINE_STATE_IDLE,                  // Idle state.
    ENGINE_STATE_HOVER,                 // A widget is being hovered but the mouse is up.
    ENGINE_STATE_PRE_DRAG_THRESHOLD,    // Inbetween mouse down and callback (checking if click or drag).
    ENGINE_STATE_POST_DRAG_THRESHOLD,   // The drag threshold has been reached but the widget can't be dragged.
    ENGINE_STATE_DRAG,                  // A widget is getting dragged and is located on the mouse.
    ENGINE_STATE_TO_DRAG,               // A widget is getting dragged and is moving towards the mouse.
    ENGINE_STATE_TO_SNAP,               // A widget is getting dragged and is mocing towards the snap.
    ENGINE_STATE_SNAP                   // A widget is getting dragged and is located on the snap.
} widget_engine_state;

static const char* engine_state_str[] = {
       "Idle",
       "Hover",
       "Click (Pre-Threshold)",
       "Click (Post-Threshold)",
       "Drag",
       "To Drag",
       "To Snap",
       "Snap"
};

static struct widget* queue_head;
static struct widget* queue_tail;

static ALLEGRO_SHADER* offscreen_shader;
static ALLEGRO_BITMAP* offscreen_bitmap;

static double transition_timestamp;

static struct widget* last_click;
static struct widget* current_hover;
static struct widget* current_drop;

static struct keyframe drag_release;
static double drag_offset_x, drag_offset_y;
static double snap_offset_x, snap_offset_y;
static const double snap_speed = 1000; // px per sec
static const double drag_threshold = 0.2;

// Prevents current_hover, last_click, and current_drag becoming stale
static void widget_prevent_stale_pointers(struct widget* const ptr)
{
    if (current_hover == ptr)
    {
        call(ptr, hover_end);

        widget_engine_state = ENGINE_STATE_IDLE;
        current_hover = NULL;
    }

    if (last_click == ptr)
    {
        call(ptr,click_off)

        last_click = NULL;
    }

    if (current_drop == ptr)
    {
        call(ptr, drag_end_no_drop)

        widget_engine_state = ENGINE_STATE_IDLE;
        current_drop = NULL;
    }
}

// Pop a widget out of the engine
void widget_interface_pop(struct widget_interface* const ptr)
{
    struct widget* const widget = (struct widget* const) ptr;

    if (widget->next)
        widget->next->previous = widget->previous;
    else
        queue_tail = widget->previous;

    if (widget->previous)
        widget->previous->next = widget->next;
    else
        queue_head = widget->next;

    // Make sure we don't get stale pointers
    widget_prevent_stale_pointers(widget);
}

// Draw the widgets in queue order.
void widget_engine_draw()
{
    style_element_setup();

    // Maybe add a second pass for stencil effect?
    for(struct widget* widget = queue_head; widget; widget = widget->next)
	{
        const struct style_element* const style_element  = widget->style_element;
        style_element_predraw(style_element);
        widget->jump_table->draw((struct widget_interface*) widget);
 	}

    if (1)
    {
        al_use_shader(NULL);
        glDisable(GL_STENCIL_TEST);
        al_use_transform(&identity_transform);

        al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 10, 10, ALLEGRO_ALIGN_LEFT, "State: %s", engine_state_str[widget_engine_state]);

        //al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 10, 10, ALLEGRO_ALIGN_LEFT, "Hover: %p", current_hover);
        //al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 10, 40, ALLEGRO_ALIGN_LEFT, "Drop: %p", current_drop);
        //al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 10, 70, ALLEGRO_ALIGN_LEFT, "Drag Release: %f %f", drag_release.x, drag_release.y);
    }
}

// Make a work queue with only the widgets that have an update method.
struct work_queue* widget_engine_widget_work()
{
    struct work_queue* work_queue = work_queue_create();

    // Since the update method doesn't change maybe we should have a static queue?
    for (struct widget* widget = queue_head; widget; widget = widget->next)
        if (widget->jump_table->update)
            work_queue_push(work_queue, widget->jump_table->update, widget);

    return work_queue;
}

// Handle picking mouse inputs using off screen drawing.
static inline struct widget* widget_engine_pick(int x, int y)
{
    ALLEGRO_BITMAP* original_bitmap = al_get_target_bitmap();

    al_set_target_bitmap(offscreen_bitmap);
    al_set_clipping_rectangle(x - 1, y - 1, 3, 3);
    glDisable(GL_STENCIL_TEST);

    al_clear_to_color(al_map_rgba(0, 0, 0, 0));

    al_use_shader(offscreen_shader);

    ALLEGRO_TRANSFORM transform;
    size_t picker_index = 1;
    size_t pick_buffer;
    float color_buffer[3];

    for (struct widget* widget = queue_head; widget; widget = widget->next, picker_index++)
    {
        if (widget == current_hover && (
            widget_engine_state == ENGINE_STATE_DRAG ||
            widget_engine_state == ENGINE_STATE_SNAP ||
            widget_engine_state == ENGINE_STATE_TO_SNAP ||
            widget_engine_state == ENGINE_STATE_TO_DRAG
            ))
        {
            continue;
        }

        pick_buffer = picker_index;

        for (size_t i = 0; i < 3; i++)
        {
            color_buffer[i] = ((float)(pick_buffer % 200)) / 200;
            pick_buffer /= 200;
        }

        al_set_shader_float_vector("picker_color", 3, color_buffer, 1);
        keyframe_build_transform(&widget->style_element->current, &transform);
        al_use_transform(&transform);
        call_engine(widget, mask);
    }

    al_set_target_bitmap(original_bitmap);

    al_unmap_rgb_f(al_get_pixel(offscreen_bitmap, x, y),
        color_buffer, color_buffer + 1, color_buffer + 2);

    size_t index = round(200 * color_buffer[0]) +
        200 * round(200 * color_buffer[1]) +
        40000 * round(200 * color_buffer[2]);

    if (index == 0)
        return NULL;

    struct widget* widget = queue_head;

    while (index-- > 1 && widget)
        widget = widget->next;

    return widget;
}

// Update the transition_timestamp based on how far the widget is to the point.
static inline void update_transition_timestamp(struct widget* widget, double x, double y)
{
    const double dx = widget->style_element->current.x - x;
    const double dy = widget->style_element->current.y - y;

    const double offset =  sqrtf(dx * dx + dy * dy)/ snap_speed;

    transition_timestamp = current_timestamp + offset;
}

// Updates current_hover keyframes to move towards the drag
static inline void widget_set_drag()
{
    style_element_interupt(current_hover->style_element);

    update_transition_timestamp(current_hover, drag_offset_x + mouse_x, drag_offset_y + mouse_y);

    struct keyframe* keyframe = style_element_new_frame(current_hover->style_element);

    keyframe->x = drag_offset_x + mouse_x;
    keyframe->y = drag_offset_y + mouse_y;
    keyframe->timestamp = transition_timestamp;
}

// Updates current_hover keyframes to move towards the drag release
static inline void widget_drag_release()
{
    style_element_interupt(current_hover->style_element);

    update_transition_timestamp(current_hover, drag_release.x, drag_release.y);
    drag_release.timestamp = transition_timestamp;

	memcpy(style_element_new_frame(current_hover->style_element),
        &drag_release,sizeof(struct keyframe));
}

// Updates and calls any callbacks for the last_click, current_hover, current_drop pointers
static inline void widget_engine_update_drag_pointers()
{
    // What pointer the picker returns depends on if something is being dragged.
    // If nothing is being dragged the pointer is what's being hovered.
    // If something is being dragged the pointer is what's under the dragged widget.
    //  (The widget under the drag is called the drop).
    struct widget* const new_pointer = widget_engine_pick(mouse_x, mouse_y);

    if (current_hover != new_pointer && (
        widget_engine_state == ENGINE_STATE_IDLE ||
        widget_engine_state == ENGINE_STATE_HOVER ))
    {
        if (current_hover)
            call(current_hover, hover_end);

        if (new_pointer)
        {
            call(new_pointer, hover_start);

            widget_engine_state = ENGINE_STATE_HOVER;
        }
        else
            widget_engine_state = ENGINE_STATE_IDLE;

        current_hover = new_pointer;
    }

    if (current_drop != new_pointer && (
        widget_engine_state == ENGINE_STATE_DRAG ||
        widget_engine_state == ENGINE_STATE_SNAP ||
        widget_engine_state == ENGINE_STATE_TO_DRAG ||
        widget_engine_state == ENGINE_STATE_TO_SNAP ))
    {
		if (current_drop)
			call(current_drop, drop_end);

		if (new_pointer)
		{
			call(new_pointer, drop_start);

			if (new_pointer->is_snappable)
			{
				style_element_interupt(current_hover->style_element);

				struct keyframe* snap_target = style_element_new_frame(current_hover->style_element);

				snap_target->x = new_pointer->style_element->current.x + snap_offset_x;
				snap_target->y = new_pointer->style_element->current.y + snap_offset_y;

                update_transition_timestamp(current_hover, snap_target->x, snap_target->y);

                snap_target->timestamp = transition_timestamp;

				widget_engine_state = ENGINE_STATE_TO_SNAP;
			}
		}
		else
		{
			widget_set_drag();
			widget_engine_state = current_drop && current_drop->is_snappable? ENGINE_STATE_TO_DRAG : ENGINE_STATE_DRAG;
		}

        current_drop = new_pointer;
    }
}

// Update the widget engine state
void widget_engine_update()
{
    widget_engine_update_drag_pointers();

    if(current_timestamp > transition_timestamp)
        switch (widget_engine_state)
        {
        case ENGINE_STATE_PRE_DRAG_THRESHOLD:
            if (current_hover->is_draggable)
            {
                call(current_hover, drag_start);

                widget_engine_state = ENGINE_STATE_DRAG;
            }
            else
            {
                widget_engine_state = ENGINE_STATE_POST_DRAG_THRESHOLD;
            }
            break;
        case ENGINE_STATE_TO_DRAG:
            widget_engine_state = ENGINE_STATE_DRAG;
            break;
        case ENGINE_STATE_TO_SNAP:
            widget_engine_state = ENGINE_STATE_SNAP;
            break;
        }

    switch (widget_engine_state)
    {
    case ENGINE_STATE_DRAG:
        current_hover->style_element->current.x = drag_offset_x + mouse_x;
        current_hover->style_element->current.y = drag_offset_y + mouse_y;
        break;

    case ENGINE_STATE_TO_DRAG:
        widget_set_drag();
        break;
    }
}

// Handle events by calling all widgets that have a event handler.
void widget_engine_event_handler()
{
    // Incorperate to the threadpool?
    for (struct widget* widget = queue_head; widget; widget = widget->next)
        call_engine(widget, event_handler);

    switch (current_event.type)
    {
    case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
        if (!current_hover)
            break;

        if (current_event.mouse.button == 2)
        {
            call(current_hover, right_click);
        }

        if (current_event.mouse.button == 1)
        {
            transition_timestamp = current_timestamp + drag_threshold;

            widget_engine_state = ENGINE_STATE_PRE_DRAG_THRESHOLD;

            drag_offset_x = current_hover->style_element->current.x - mouse_x;
            drag_offset_y = current_hover->style_element->current.y - mouse_y;
            style_element_copy_destination(current_hover->style_element, &drag_release);

            if (current_hover != last_click)
            {
                if (last_click)
                    call(last_click, click_off);

                last_click = current_hover;
            }
        }
        break;

    case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
        if (current_event.mouse.button != 1 || !current_hover)
            break;

		switch(widget_engine_state)
		{ 
        case ENGINE_STATE_PRE_DRAG_THRESHOLD:
            call(current_hover, left_click);
            break;

        case ENGINE_STATE_DRAG:
        case ENGINE_STATE_SNAP:
        case ENGINE_STATE_TO_SNAP:
        case ENGINE_STATE_TO_DRAG:
            if (current_drop)
            {
                if(current_drop->jump_table->drag_end_drop)
                    call_va(current_drop, drag_end_drop, (struct widget_interface*) current_hover);
                else
                    call(current_hover, drag_end_no_drop);

                call_lua(current_drop, drag_end_drop)
            }
            else
                call(current_hover, drag_end_no_drop);

            widget_drag_release();
            break;
        }

        widget_engine_state = current_hover ? ENGINE_STATE_HOVER : ENGINE_STATE_IDLE;
        break;
    }
}

// Allocate a new widget interface and wire it into the widget engine.
struct widget_interface* widget_interface_new(
    lua_State* L,
    const void* const upcast,
    const struct widget_jump_table* const jump_table )
{
    const size_t widget_size = sizeof(struct widget);

    struct widget* const widget = L ? lua_newuserdata(L, widget_size) : malloc(widget_size);

    if (!widget)
        return NULL;

    *widget = (struct widget)
    {
        .upcast = upcast,
        .style_element = style_element_new(1),

        .jump_table = jump_table,

#define CLEAR_LUA_REFNIL(method) .lua. ## method = LUA_REFNIL,
        FOR_CALLBACKS(CLEAR_LUA_REFNIL)

        .next = NULL,
        .previous = queue_tail,
        .is_draggable = false,
        .is_snappable = false
    };

    if (queue_tail)
        queue_tail->next = widget;
    else
        queue_head = widget;

    queue_tail = widget;

    luaL_getmetatable(L, "widget_mt");
    lua_setmetatable(L, -2);

    return (struct widget_interface*) widget;
}

// Check that the data at the given index is a widget and has the given jumptable.
struct widget_interface* check_widget(lua_State* L, int idx, const struct widget_jump_table* const jump_table)
{
    const struct widget* const widget = (struct widget*)luaL_checkudata(L, idx, "widget_mt");
    return (widget->jump_table == jump_table) ? (struct widget_interface*)widget : NULL;
}

// Widget methods

static void inline read_transform(lua_State* L, struct keyframe* keyframe)
{
    luaL_checktype(L, -1, LUA_TTABLE);

#define READ(member,...)     lua_pushstring(L, #member); \
    if (LUA_TNUMBER == lua_gettable(L, idx--)) keyframe-> ## member = luaL_checknumber(L, -1);

    int idx = -2;

    FOR_KEYFRAME_NUMBER_MEMBERS(READ)

    lua_settop(L, lua_gettop(L) - 6);
}

static void inline write_transform(lua_State* L, struct keyframe* keyframe)
{
    // optimize for tail call? or is inline enough
    lua_createtable(L, 0, 8);

#define WRITE(member,...) lua_pushstring(L, #member); lua_pushnumber(L, keyframe-> ## member); lua_settable(L,-3);
    FOR_KEYFRAME_NUMBER_MEMBERS(WRITE)

    return;
}

static int set_keyframe(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -2, "widget_mt");

    struct keyframe keyframe;

    keyframe_default(&keyframe);
    read_transform(L, &keyframe);

    style_element_set(widget->style_element, &keyframe);

    return 0;
}

static int current_keyframe(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -2, "widget_mt");
    const struct keyframe* const keyframe = &widget->style_element->current;

    write_transform(L, keyframe);

    return 1;
}

static int destination_keyframe(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -2, "widget_mt");

    struct keyframe keyframe;
    style_element_copy_destination(widget->style_element, &keyframe);

    write_transform(L, &keyframe);

    return 1;
}

static int new_keyframe(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -2, "widget_mt");
	luaL_checktype(L, -1, LUA_TTABLE);

	struct keyframe* keyframe = style_element_new_frame(widget->style_element);

	read_transform(L, keyframe);
	return 0;
}

static int interupt(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -1, "widget_mt");

    style_element_interupt(widget->style_element);

    return 0;
}

// gc
static int gc(lua_State* L)
{
    struct widget* const widget = (struct widget*)luaL_checkudata(L, 1, "widget_mt");

    call_engine(widget, gc);
    widget_interface_pop((struct widget_interface* const) widget);

    return 0;
}

// test index
static int index(lua_State* L)
{
    struct widget* const widget = (struct widget*)luaL_checkudata(L, -2, "widget_mt");
    
    // Ineficient, should use a hash or BST 
    if (lua_type(L, -1) == LUA_TSTRING)
    {

        const char* key = lua_tostring(L, -1);

#define PUSH_FUNC_CALL(function) if (strcmp(#function, key) == 0){ lua_pushcfunction(L, function); return 1;} else
#define PUSH_VALUE_CALL(function) if (strcmp(#function, key) == 0){ return destination_keyframe(L);} else

            PUSH_FUNC_CALL(set_keyframe)
            PUSH_VALUE_CALL(destination_keyframe)
            PUSH_VALUE_CALL(current_keyframe)
            PUSH_FUNC_CALL(new_keyframe)
            PUSH_FUNC_CALL(interupt);
    }

    if (widget->jump_table->index)
        return widget->jump_table->index(L);

    return 0;
}

static int newindex(lua_State* L)
{
    struct widget* const widget = (struct widget*)luaL_checkudata(L, -3, "widget_mt");

    // This is inefficient but can be optimized once a set of callbacks it confirmed
    if (lua_type(L, -1) == LUA_TFUNCTION &&
        lua_type(L, -2) == LUA_TSTRING)
    {
        const char* key = lua_tostring(L, -2);

#define CHECK(callback,...) if(strcmp(key, #callback) == 0)\
    {\
        widget->lua. ## callback = luaL_ref(L,LUA_REGISTRYINDEX); \
        return 0; \
    }\

        FOR_CALLBACKS(CHECK)
    }

    if (widget->jump_table->newindex)
        return widget->jump_table->newindex(L);

    return 0;
}

#define FOR_WIDGETS(DO) \
	DO(rectangle) \
	//DO(card) \

#define EXTERN(widget) extern int widget ## _new(lua_State*);

FOR_WIDGETS(EXTERN)

int get_current_time(lua_State* L)
{
    lua_pushnumber(L, current_timestamp);
    lua_pushnumber(L, delta_timestamp);

    return 2;
}

// make metatable
static void inline make_meta_table(lua_State* L)
{
    // Push the main metatable to the stack
    luaL_newmetatable(L, "widget_mt");

    // Set the garbage collector to the main metatable
    const struct luaL_Reg garbage_collection[] = {
        {"__gc",gc},
        {"__newindex",newindex},
        {"__index",index},
        {NULL,NULL}
    };

    luaL_setfuncs(L, garbage_collection, 0);

    if (0)
    {
        // Make the __index table
        lua_newtable(L);

        const struct luaL_Reg widget_transform_methods[] = {
            {"set_keyframe",set_keyframe},
            {"current_keyframe",current_keyframe},
            // 	{"destination_keyframe",destination_keyframe},
                {"new_keyframe",new_keyframe},
                {"interupt",interupt},
                {"__index",index},
                {NULL,NULL}
        };

        luaL_setfuncs(L, widget_transform_methods, 0);

        // Set the __index table to the main meta table
        lua_setfield(L, -2, "__index");
    }
}

static void inline widget_lua_integration(lua_State* L)
{
    #define LUA_REG_ENTRY(widget) {#widget , widget ## _new},

    const struct luaL_Reg lib_f[] = {
        FOR_WIDGETS(LUA_REG_ENTRY)
        {"current_time",get_current_time},
        {NULL,NULL}
    };

    luaL_newlib(L, lib_f);
    lua_setglobal(L, "widget_engine");

    make_meta_table(L);
}

// Initalize the widget engine
void widget_engine_init(lua_State* L)
{
    queue_head = NULL;
    queue_tail = NULL;

    current_drop = NULL;
    current_hover = NULL;
    last_click = NULL;

    snap_offset_x = 5;
    snap_offset_y = 5;

    offscreen_shader = al_create_shader(ALLEGRO_SHADER_GLSL);

    if (!al_attach_shader_source_file(offscreen_shader, ALLEGRO_VERTEX_SHADER, "widget_vertex_shader.glsl"))
    {
        fprintf(stderr, "Failed to attach vertex shader.\n%s\n", al_get_shader_log(offscreen_shader));
        return;
    }

    if (!al_attach_shader_source_file(offscreen_shader, ALLEGRO_PIXEL_SHADER, "widget_pixel_shader.glsl"))
    {
        fprintf(stderr, "Failed to attach pixel shader.\n%s\n", al_get_shader_log(offscreen_shader));
        return;
    }

    if (!al_build_shader(offscreen_shader))
    {
        fprintf(stderr, "Failed to build shader.\n%s\n", al_get_shader_log(offscreen_shader));
        return;
    }

    offscreen_bitmap = al_create_bitmap(
        al_get_bitmap_width(al_get_target_bitmap()),
        al_get_bitmap_width(al_get_target_bitmap()));

    widget_lua_integration(L);
}

