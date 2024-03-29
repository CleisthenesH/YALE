// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include "thread_pool.h"
#include "camera.h"

#include <allegro5/allegro_font.h>
#include <allegro5/allegro_opengl.h>

#include <stdio.h>
#include <float.h>
#include <math.h>

// Debug Macro Flags
//#define WIDGET_DEBUG_DRAW
//#define WIDGET_EVENT_LOG

#ifdef WIDGET_DEBUG_DRAW
#include <allegro5/allegro_primitives.h>
void material_apply(const struct material* const);
#endif

#ifdef WIDGET_EVENT_LOG
#include <stdio.h>
#endif

// widget_interface.c exclusive render_interface.c methods
extern void render_interface_global_predraw();
extern void render_interface_use_transform(const struct render_interface* const);
extern void render_interface_predraw(const struct render_interface* const);

// Global Variables
extern double mouse_x;
extern double mouse_y;
extern double current_timestamp;
extern double delta_timestamp;
extern const ALLEGRO_FONT* debug_font;
extern ALLEGRO_EVENT current_event;
extern const ALLEGRO_TRANSFORM identity_transform;
extern void invert_transform_3D(ALLEGRO_TRANSFORM*);
extern lua_State* const main_lua_state;

/*********************************************/
/*        Internal Widget Interface          */
/*********************************************/

// CALLBACK X macro 
// columns are method,argcnt,offset
#define FOR_CALLBACKS(DO) \
	DO(right_click,1,0) \
	DO(left_click,1,1) \
    DO(left_click_end,1,2) \
	DO(hover_start,1,3) \
	DO(hover_end,1,4) \
	DO(click_off,1,5) \
	DO(drag_start,1,6) \
	DO(drag_end_drop,1,7) \
	DO(drag_end_no_drop,2,8) \
	DO(drop_start,2,9) \
	DO(drop_end,2,10) \

enum WIDGET_UPVALUE
{
    WIDGET_UPVALUE_START = 0,
};

struct widget
{
	struct widget_interface;
    const struct widget_jump_table* jump_table;

    struct widget* next;
    struct widget* previous;

    // TODO: Manage Registry Use
    // This method creates a new registry entry for every function call.
    // I don't want to prematurly optimize it away but if it becomes to much it can be replace by a single table.
    // And if that become's too much a single registry entryf or each widget. 
    // And if that's too much a sperate table for all active widgets.
    // (This could also have other uses + it should be a weak table to not interfer with gc).
    // Q: Why not use uservalues. A: The callback can be called from anycontext, i.e. not knowing the udata.
    // 
    // Maype some look up
    struct
    {
#define DECLARE(method,...) int method;
        FOR_CALLBACKS(DECLARE)
    } lua;
};

// TODO: implement as a hash so we don't have to use macros
// but hashing is still runtime maybe enumerations would be better
#define call_lua(widget,method) if(widget->lua. ## method != LUA_REFNIL) do{ \
    lua_rawgeti(main_lua_state, LUA_REGISTRYINDEX, widget->lua. ## method); \
	lua_getglobal(main_lua_state, "widgets"); \
	lua_rawgetp(main_lua_state, -1, widget); \
	lua_remove(main_lua_state, -2); \
    if(lua_pcall(main_lua_state, 1, 0, 0) != LUA_OK) \
    {\
        printf("Error calling method \"" #method "\" on widget \"" #widget"\".\n\tFrom " __FILE__ " at line %d.\n\n\t%s\n\n", \
        __LINE__, luaL_checkstring(main_lua_state, -1)); \
    }\
 } while(0); 

#define call_engine(widget,method) if(widget->jump_table-> ## method) \
    do{ \
        (widget)->jump_table-> ## method((struct widget_interface*) (widget)); \
    }while(0);

#ifdef WIDGET_EVENT_LOG
#define call(widget,method) \
    do{printf("WIDGET EVENT method: " #method "\t widget: %p \n", widget);call_engine(widget,method) call_lua(widget,method)}while(0);
#else
#define call(widget,method) \
    do{call_engine(widget,method) call_lua(widget,method)}while(0);
#endif

#ifdef WIDGET_EVENT_LOG
#define call_va(widget,method,...) \
    do{printf("WIDGET EVENT method: " #method "\t widget %p\t args:" #__VA_ARGS__ "\n",widget); (widget)->jump_table->method((struct widget_interface*) (widget),__VA_ARGS__);}while(0)
#else
#define call_va(widget,method,...) (widget)->jump_table->method((struct widget_interface*) (widget),__VA_ARGS__)
#endif

/*********************************************/
/*           Widget Engine State             */
/*********************************************/

static enum {
    ENGINE_STATE_IDLE,                  // Idle state.
    ENGINE_STATE_HOVER,                 // A widget is being hovered but the mouse is up.
    ENGINE_STATE_PRE_DRAG_THRESHOLD,    // Inbetween mouse down and callback (checking if click or drag).
    ENGINE_STATE_POST_DRAG_THRESHOLD,   // The drag threshold has been reached but the widget can't be dragged.
    ENGINE_STATE_DRAG,                  // A widget is getting dragged and is located on the mouse.
    ENGINE_STATE_TO_DRAG,               // A widget is getting dragged and is moving towards the mouse.
    ENGINE_STATE_TO_SNAP,               // A widget is getting dragged and is mocing towards the snap.
    ENGINE_STATE_SNAP,                  // A widget is getting dragged and is located on the snap.
    ENGINE_STATE_EMPTY_DRAG,            // Drag but without a widget underneath. Used to control camera.
    ENGINE_STATE_LOCKED,                // The widget engine has been locked.
} widget_engine_state;

static double transition_timestamp;

static const char* engine_state_str[] = {
       "Idle",
       "Hover",
       "Click (Pre-Threshold)",
       "Click (Post-Threshold)",
       "Drag",
       "To Drag",
       "To Snap",
       "Snap",
       "Empty Drag",
       "Locked"
};

/*********************************************/
/*          Widget Queue Methods             */
/*********************************************/

static struct widget* queue_head;
static struct widget* queue_tail;
static struct widget* lock;

// Pop a widget out of the engine
static void queue_pop(struct widget_interface* const ptr)
{
    struct widget* const widget = (struct widget* const)ptr;

    if (widget->next)
        widget->next->previous = widget->previous;
    else
        queue_tail = widget->previous;

    if (widget->previous)
        widget->previous->next = widget->next;
    else
        queue_head = widget->next;

    if (ptr == (struct widget_interface* const) lock)
        lock = widget->next;
}

// Insert the first widget behind the second
static void queue_insert(struct widget_interface* mover, struct widget_interface* target)
{
    // Assumes mover is outside the list.
    // I.e. that mover's next and previous are null.

    struct widget* internal_target = (struct widget*)target;
    struct widget* internal_mover = (struct widget*)mover;

    internal_mover->next = internal_target;

    // If the second is null append the first to the head
    if (internal_target)
    {
        if (internal_target == queue_head)
            lock = internal_target;

        if (internal_target->previous)
            internal_target->previous->next = internal_mover;
        else
            queue_tail = internal_mover;


        internal_target->previous = internal_mover;
    }
    else
    {
        internal_mover->previous = queue_tail;

        if (queue_tail)
            queue_tail->next = internal_mover;
        else
            queue_head = internal_mover;
 
        queue_tail = internal_mover;
    }
}

// Move the mover widget behind the target widget.
void widget_interface_move(struct widget_interface* mover, struct widget_interface* target)
{
    if (mover == target)
        return;

    // Only this function is visable to the widget writer.
    // Since poping a widget without calling gc isn't allowed.
    queue_pop(mover);
    queue_insert(mover, target);
}

/*********************************************/
/*      General Widget Engine Methods        */
/*********************************************/

// Click, Drag, and Drop pointers
static struct widget* last_click;
static struct widget* current_hover;
static struct widget* current_drop;

// Drag and Snap Variables
static struct keyframe drag_release;
static double drag_offset_x, drag_offset_y; // coordinates system match current_hover (screen or world)
static double snap_offset_x, snap_offset_y; // coordinates system match snap (screen or world)
static const double snap_speed = 1000; // px per sec
static const double drag_threshold = 0.2;

// Pick (Offscreen drawing) Variables
static ALLEGRO_SHADER* offscreen_shader;
static ALLEGRO_BITMAP* offscreen_bitmap;

// Miscellaneous 
static int contex_callback = LUA_REFNIL;

// Prevents current_hover, last_click, and current_drag becoming stale
static void prevent_stale_pointers(struct widget* const ptr)
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

// Whether the current_hover should be rendered ontop of other widgets.
static bool hover_on_top()
{
    return widget_engine_state == ENGINE_STATE_DRAG ||
        widget_engine_state == ENGINE_STATE_SNAP ||
        widget_engine_state == ENGINE_STATE_TO_SNAP ||
        widget_engine_state == ENGINE_STATE_TO_DRAG;
}

// Draws the given widget.
static void draw_widget(const struct widget* const widget)
{
    const struct render_interface* const render_interface = widget->render_interface;
    render_interface_predraw(render_interface);
    widget->jump_table->draw((struct widget_interface*)widget);
#ifdef WIDGET_DEBUG_DRAW
    const double half_width = render_interface->half_width;
    const double half_height = render_interface->half_height;

    material_apply(NULL);
    al_draw_rectangle(-half_width, -half_height, half_width, half_height, al_map_rgb_f(0, 1, 0), 1);
#endif
}

// Update the transition_timestamp based on how far the widget is to the point.
static inline void update_transition_timestamp(struct widget* widget, double x, double y)
{
    // DEPREICATED BECAUSE I HAVENT THOUGHT ABOUT CORD_SYS LOGIC YET
    const double dx = widget->render_interface->current.dx - x;
    const double dy = widget->render_interface->current.dy - y;

    const double offset =  sqrtf(dx * dx + dy * dy)/ snap_speed;

    transition_timestamp = current_timestamp;//fasdf +offset;
}

// Move the current_hover towards the drag location
static inline void towards_drag()
{
    struct keyframe keyframe = drag_release;

    render_interface_interupt(current_hover->render_interface);

    keyframe.dx = mouse_x - drag_offset_x;
    keyframe.dy = mouse_y - drag_offset_y;
    keyframe.timestamp = current_timestamp + 0.1;

    render_interface_push_keyframe(current_hover->render_interface, &keyframe);
}

// Handle picking mouse inputs using off screen drawing.
static inline struct widget* pick(int x, int y)
{
    ALLEGRO_BITMAP* original_bitmap = al_get_target_bitmap();

    al_set_target_bitmap(offscreen_bitmap);
    al_set_clipping_rectangle(x - 1, y - 1, 3, 3);
    glDisable(GL_STENCIL_TEST);

    al_clear_to_color(al_map_rgba(0, 0, 0, 0));

    al_use_shader(offscreen_shader);

    size_t picker_index = 1;
    size_t pick_buffer;
    float color_buffer[3];

    const bool hide_hover = hover_on_top();

    if (hide_hover)
        queue_pop((struct widget_interface*)current_hover);

    for (struct widget* widget = lock; widget; widget = widget->next, picker_index++)
    {
        pick_buffer = picker_index;

        for (size_t i = 0; i < 3; i++)
        {
            color_buffer[i] = ((float)(pick_buffer % 200)) / 200;
            pick_buffer /= 200;
        }

        al_set_shader_float_vector("picker_color", 3, color_buffer, 1);
        render_interface_use_transform(widget->render_interface);

        call_engine(widget, mask);
    }


    al_set_target_bitmap(original_bitmap);

    al_unmap_rgb_f(al_get_pixel(offscreen_bitmap, x, y),
        color_buffer, color_buffer + 1, color_buffer + 2);

    size_t index = round(200 * color_buffer[0]) +
        200 * round(200 * color_buffer[1]) +
        40000 * round(200 * color_buffer[2]);

    if (index == 0)
    {
        if (hide_hover)
            queue_insert((struct widget_interface*)current_hover, (struct widget_interface*)current_hover->next);

        return NULL;
    }

    struct widget* widget = lock;

    while (index-- > 1 && widget)
        widget = widget->next;

    if (hide_hover)
        queue_insert((struct widget_interface*)current_hover, (struct widget_interface*)current_hover->next);

    return widget;
}

// Updates and calls any callbacks for the last_click, current_hover, current_drop pointers
static inline void update_drag_pointers()
{
    // What pointer the picker returns depends on if something is being dragged.
    // If nothing is being dragged the pointer is what's being hovered.
    // If something is being dragged the pointer is what's under the dragged widget.
    //  (The widget under the drag is called the drop).
    struct widget* const new_pointer = pick(mouse_x, mouse_y);

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

        return;
    }

    if (current_drop != new_pointer && (
        widget_engine_state == ENGINE_STATE_DRAG ||
        widget_engine_state == ENGINE_STATE_SNAP ||
        widget_engine_state == ENGINE_STATE_TO_DRAG ||
        widget_engine_state == ENGINE_STATE_TO_SNAP ))
    {
        if (current_drop)
        {
            if (current_drop->jump_table->drop_end)
            {
                call_va(current_drop, drop_end, (struct widget_interface*)current_hover);
            }
            call_lua(current_drop, drop_end)
        }

		if (new_pointer)
		{
            if (new_pointer->jump_table->drop_start)
            {
                call_va(new_pointer, drop_start, (struct widget_interface*)current_hover);
            }
            call_lua(new_pointer, drop_start)

			if (new_pointer->is_snappable)
			{
				render_interface_interupt(current_hover->render_interface);

				struct keyframe snap_target;
				render_interface_copy_destination(new_pointer->render_interface, &snap_target);

				snap_target.dx += snap_offset_x;
				snap_target.dy += snap_offset_y;
                snap_target.timestamp = current_timestamp + 0.1;

				render_interface_push_keyframe(current_hover->render_interface, &snap_target);

				widget_engine_state = ENGINE_STATE_TO_SNAP;
			}
            else
            {
                towards_drag();
                widget_engine_state = ENGINE_STATE_TO_DRAG;
            }
		}
		else
		{
            if (current_drop && current_drop->is_snappable)
            {
                towards_drag();
                widget_engine_state = ENGINE_STATE_TO_DRAG;
            }
            else
				widget_engine_state = ENGINE_STATE_DRAG;
		}

        current_drop = new_pointer;

        return;
    }
}

// Convert a screen position to the cordinate used when drawng
void widget_screen_to_local(const struct widget_interface* const widget, double* x, double* y)
{
    ALLEGRO_TRANSFORM transform;

    // The allegro uses float but standards have moved forward to doubles.
    // This is the easist solution.
    float _x = *x;
    float _y = *y;

    keyframe_build_transform(&widget->render_interface->current, &transform);

    // WARNING: the inbuilt invert only works for 2D transforms
    if (1)
        al_invert_transform(&transform);
    else
        invert_transform_3D(&transform);


    al_transform_coordinates(&transform, &_x, &_y);

    *x = _x;
    *y = _y;
}

// Check that a widget has the right jumptable
struct widget_interface* check_widget(struct widget_interface* widget, const struct widget_jump_table* const jump_table)
{
    return (((struct widget*)widget)->jump_table == jump_table) ? (struct widget_interface*) widget : NULL;
}

/*********************************************/
/*            Big Four Callbacks             */
/*********************************************/

// Draw the widgets in queue order.
void widget_engine_draw()
{
    render_interface_global_predraw();

    const bool hide_hover = hover_on_top();

    if (hide_hover)
        queue_pop((struct widget_interface*)current_hover);

    // Maybe add a second pass for stencil effect?
    for (struct widget* widget = queue_head; widget; widget = widget->next)
        draw_widget(widget);

    if (hide_hover)
    {
        queue_insert((struct widget_interface*)current_hover, (struct widget_interface*)current_hover->next);
        draw_widget(current_hover);
    }

#ifdef WIDGET_DEBUG_DRAW
    al_use_shader(NULL);
    glDisable(GL_STENCIL_TEST);
    al_use_transform(&identity_transform);

    al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 10, 10, ALLEGRO_ALIGN_LEFT,
        "State: %s", engine_state_str[widget_engine_state]);
    al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 10, 40, ALLEGRO_ALIGN_LEFT,
        "Hover: %p", current_hover);
    al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 10, 70, ALLEGRO_ALIGN_LEFT,
        "Drop: %p", current_drop);
    al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 10, 100, ALLEGRO_ALIGN_LEFT,
        "Drag Release: (%f, %f) %f (%f, %f)", drag_release.x, drag_release.y, 
        drag_release.timestamp, drag_release.dx, drag_release.dy);
#endif
}

// Update the widget engine state
void widget_engine_update()
{
    if (widget_engine_state == ENGINE_STATE_LOCKED)
        return;

    update_drag_pointers();

    if(current_timestamp > transition_timestamp)
        switch (widget_engine_state)
        {
        case ENGINE_STATE_PRE_DRAG_THRESHOLD:
            if (current_hover->is_draggable)
            {
                call(current_hover, drag_start);

                towards_drag();
                widget_engine_state = ENGINE_STATE_TO_DRAG;
            }
            else
            {
                call(current_hover, left_click);

                widget_engine_state = ENGINE_STATE_POST_DRAG_THRESHOLD;
            }
            break;
        case ENGINE_STATE_TO_DRAG:
            widget_engine_state = ENGINE_STATE_DRAG;

            towards_drag();
            break;

        case ENGINE_STATE_TO_SNAP:
            widget_engine_state = ENGINE_STATE_SNAP;
            break;
        }

    switch (widget_engine_state)
    {
    case ENGINE_STATE_DRAG:
    {
        struct keyframe buffer = drag_release;

        buffer.dx = mouse_x - drag_offset_x;
        buffer.dy = mouse_y - drag_offset_y;

        render_interface_set(current_hover->render_interface, &buffer);

        break;
    }

    case ENGINE_STATE_TO_DRAG:
    {
        towards_drag();

        break;
    }
    }
}

// Make a work queue with only the widgets that have an update method.
struct work_queue* widget_engine_widget_work()
{
    struct work_queue* work_queue = work_queue_create();

    // Since the update method doesn't change maybe we should have a static queue?
    if (widget_engine_state != ENGINE_STATE_LOCKED)
		for (struct widget* widget = lock; widget; widget = widget->next)
			if (widget->jump_table->update)
				work_queue_push(work_queue, widget->jump_table->update, widget);

    return work_queue;
}

// Handle events by calling all widgets that have a event handler.
void widget_engine_event_handler()
{
    // TODO: Incorperate to the threadpool?
    if(widget_engine_state != ENGINE_STATE_LOCKED)
		for (struct widget* widget = lock; widget; widget = widget->next)
			call_engine(widget, event_handler);

    switch (current_event.type)
    {
    case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
        if (current_hover != last_click)
        {
            if (last_click)
                call(last_click, click_off);

            last_click = current_hover;
        }

        if (!current_hover)
        {
            if (widget_engine_state == ENGINE_STATE_IDLE)
                if(current_event.mouse.button == 1)
				{
					widget_engine_state = ENGINE_STATE_EMPTY_DRAG;

					camera_interupt();
					camera_copy_destination(&drag_release);
				}
                else if (current_event.mouse.button == 2 &&
                    contex_callback != LUA_REFNIL)
                {
                    lua_rawgeti(main_lua_state, LUA_REGISTRYINDEX, contex_callback);
                    lua_pushnumber(main_lua_state, mouse_x);
                    lua_pushnumber(main_lua_state, mouse_y);

                    lua_pcall(main_lua_state, 2, 0, 0);
                }

            break;
        }

        if (current_event.mouse.button == 2)
        {
            call(current_hover, right_click);
        }

        if (current_event.mouse.button == 1)
        {
            transition_timestamp = current_timestamp + drag_threshold;

            widget_engine_state = ENGINE_STATE_PRE_DRAG_THRESHOLD;

            drag_offset_x = mouse_x - current_hover->render_interface->current.dx;
            drag_offset_y = mouse_y - current_hover->render_interface->current.dy;

            render_interface_copy_destination(current_hover->render_interface, &drag_release);
        }
        break;

    case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
        if (current_event.mouse.button != 1 )
            break;

        if (widget_engine_state == ENGINE_STATE_EMPTY_DRAG)
        {
            widget_engine_state = ENGINE_STATE_IDLE;
            break;
        }

        // Split up just in case empty_drag and current_hover desync
        if (!current_hover)
            break;

		switch(widget_engine_state)
		{ 
        case ENGINE_STATE_PRE_DRAG_THRESHOLD:
            call(current_hover, left_click);
            call(current_hover, left_click_end);

            // This means hover_start can be called twice without a hover_end
            // Not commited to it (should filter for existance of drag_start?)
            if (current_hover)
                call(current_hover, hover_start);
            break;

        case ENGINE_STATE_POST_DRAG_THRESHOLD:
            call(current_hover, left_click_end);

            // This means hover_start can be called twice without a hover_end
            // Not commited to it
            if (current_hover)
                call(current_hover, hover_start);
            break;

        case ENGINE_STATE_DRAG:
        case ENGINE_STATE_SNAP:
        case ENGINE_STATE_TO_SNAP:
        case ENGINE_STATE_TO_DRAG:
            render_interface_interupt(current_hover->render_interface);

            drag_release.timestamp = current_timestamp + 0.1;

            render_interface_push_keyframe(current_hover->render_interface, &drag_release);

            if (current_drop)
            {
                if (current_drop->jump_table->drag_end_drop)
                    call_va(current_drop, drag_end_drop, (struct widget_interface*)current_hover);
                else
                    call(current_hover, drag_end_no_drop);

                call_lua(current_drop, drag_end_drop)
            }
            else
                call(current_hover, drag_end_no_drop);

            current_drop = NULL;
            break;
        }

        // TODO: Pretty sure not checking here is causing flickering
        widget_engine_state = current_hover ? ENGINE_STATE_HOVER : ENGINE_STATE_IDLE;
        break;

    case ALLEGRO_EVENT_MOUSE_AXES:
        if (widget_engine_state == ENGINE_STATE_EMPTY_DRAG)
        {
            drag_release.x += current_event.mouse.dx;
            drag_release.y += current_event.mouse.dy;

            drag_release.sx += 0.01*current_event.mouse.dz;
            drag_release.sy += 0.01*current_event.mouse.dz;

            drag_release.sx = drag_release.sx < 0.2 ? 0.2:drag_release.sx;
            drag_release.sy = drag_release.sy < 0.2 ? 0.2 : drag_release.sy;

            camera_set_keyframe(&drag_release);
        }
    }
}

/*********************************************/
/*               LUA interface               */
/*********************************************/

// Check that the data at the given index is a widget and has the given jumptable.
struct widget_interface* check_widget_lua(int idx, const struct widget_jump_table* const jump_table)
{
    if (lua_type(main_lua_state, idx) != LUA_TUSERDATA)
        return NULL;

    struct widget_interface* widget = (struct widget_interface*)luaL_checkudata(main_lua_state, idx, "widget_mt");
    return widget? check_widget(widget, jump_table) : NULL;
}

// Set the widget keyframe (singular) clears all current keyframes 
static int set_keyframe(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -2, "widget_mt");

    struct keyframe keyframe;

    keyframe_default(&keyframe);
    lua_tokeyframe(L, &keyframe);

    render_interface_set(widget->render_interface, &keyframe);

    return 0;
}

// Set the widget keyframes (multiple) clears all current keyframes 
static int set_keyframes(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -2, "widget_mt");
    struct keyframe keyframe;

    lua_geti(L, -1, 1);

    keyframe_default(&keyframe);
    lua_tokeyframe(L, &keyframe);

    render_interface_set(widget->render_interface, &keyframe);
    
    lua_settop(L,-2);

    lua_geti(L, -1, 1);

    for(int i = 3;lua_istable(L,-1);i++)
    {
        keyframe_default(&keyframe);
        lua_tokeyframe(L, &keyframe);

        lua_settop(L, -2);

        render_interface_push_keyframe(widget->render_interface, &keyframe);
        
        lua_geti(L, -1, i);
    }

    return 0;
}

// Get the current keyframe
static int current_keyframe(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -2, "widget_mt");
    struct keyframe* const keyframe = &widget->render_interface->current;

    lua_pushkeyframe(L, keyframe);

    return 1;
}

// Get the destination keyframe
static int destination_keyframe(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -2, "widget_mt");

    struct keyframe keyframe;
    render_interface_copy_destination(widget->render_interface, &keyframe);

    lua_pushkeyframe(L, &keyframe);

    return 1;
}

// Reads a transform from the stack and appends to the end of its current path
static int push_keyframe(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -2, "widget_mt");
	luaL_checktype(L, -1, LUA_TTABLE);

    struct keyframe keyframe;
    keyframe_default(&keyframe);

    lua_tokeyframe(L,&keyframe);

    render_interface_push_keyframe(widget->render_interface, &keyframe);
	return 0;
}

// Interupts the widgets keyframe, clears the keyframe queue, and sets the current keyframe to the interupted value
static int interupt(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -1, "widget_mt");

    render_interface_interupt(widget->render_interface);

    return 0;
}

// Make the widget loop
static int enter_loop(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -2, "widget_mt");
    const double looping_offset = luaL_checknumber(L, -1);

    render_interface_enter_loop(widget->render_interface, looping_offset);

    return 0;
}

// Genearl widget garbage collection
static int gc(lua_State* L)
{
    struct widget* const widget = (struct widget*)luaL_checkudata(L, 1, "widget_mt");

    call_engine(widget, gc);
    queue_pop((struct widget_interface* const) widget);

    // Make sure we don't get stale pointers
    prevent_stale_pointers(widget);

    return 0;
}

// Make Snappable
static int snappable(lua_State* L)
{
    struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -3, "widget_mt");
    int boolean = lua_toboolean(L, -1);

    widget->is_snappable = boolean;

    return 0;
}

// Lua wrapper for the widget_interface_move
static int widget_move_lua(lua_State* L)
{
    struct widget_interface* const mover = (struct widget_interface*)luaL_checkudata(L, -2, "widget_mt");

    if (mover)
    {
        if (lua_isnil(L, -1))
            widget_interface_move(mover, NULL);
        else
        {
            struct widget_interface* const target = (struct widget_interface*)luaL_checkudata(L, -1, "widget_mt");
            widget_interface_move(mover, target);
        }
    }

    lua_pop(L, 2);

    return 0;
}

// Set context menu callback
static int set_context_menu(lua_State* L)
{
    if (lua_type(L, -1) == LUA_TFUNCTION)
		contex_callback = luaL_ref(L, LUA_REGISTRYINDEX);

    return 0;
}

// Lock the engine
static int engine_lock(lua_State* L)
{
    if (LUA_TUSERDATA == lua_type(L, -1))
        lock = (struct widget*)luaL_checkudata(L, -1, "widget_mt");
    else
        lock = NULL;

    return 0;
}

// Unlock the engine
static int engine_unlock(lua_State* L)
{
    lock = queue_head;

    return 0;
}

// Geneal widget index method
static int index(lua_State* L)
{
    static const struct {
        const char* key;
        const lua_CFunction function;
        const enum
        {
            CALL_PUSH_CFUNCT,
            CALL_CALL_FUNCT
        } call_behaviour;
    } index_aa[] = {
        {"set_keyframe",set_keyframe,CALL_PUSH_CFUNCT},
        {"set_keyframes",set_keyframe,CALL_PUSH_CFUNCT},
        {"destination_keyframe",destination_keyframe,CALL_CALL_FUNCT},
        {"current_keyframe",current_keyframe,CALL_CALL_FUNCT},
        {"push_keyframe",push_keyframe,CALL_PUSH_CFUNCT},
        {"interupt",interupt,CALL_PUSH_CFUNCT},
        {"enter_loop",enter_loop,CALL_PUSH_CFUNCT},
        {NULL,NULL,CALL_PUSH_CFUNCT},
    };

    struct widget* const widget = (struct widget*)luaL_checkudata(L, -2, "widget_mt");
    
    if (lua_type(L, -1) == LUA_TSTRING)
    {
        const char* key = lua_tostring(L, -1);

        for (size_t i = 0; index_aa[i].key; i++)
            if(strcmp(index_aa[i].key, key) == 0)
            {
                if(index_aa[i].call_behaviour == CALL_PUSH_CFUNCT)
                {
                    lua_pushcfunction(L, index_aa[i].function);
                    return 1;
                }

                if (index_aa[i].call_behaviour == CALL_CALL_FUNCT)
                {
                    return index_aa[i].function(main_lua_state);
                }
            }
    }

    if (widget->jump_table->index)
        return widget->jump_table->index(L);

    return 0;
}

// Geneal widget newindex method
static int newindex(lua_State* L)
{
#define CALLBACK_ENTRY(callback,_,offset) {#callback,CALL_SET_CALLBACK, offset, NULL},

    const struct {
        const char* key;
        const enum
        {
            CALL_SET_CALLBACK,
            CALL_NEWINDEX_FUNCT
        } call_behaviour;

        int callback_offset;
        int (*funct)(lua_State*);
    } newindex_aa[] = {
        {"snappable",CALL_NEWINDEX_FUNCT,0,snappable},
        FOR_CALLBACKS(CALLBACK_ENTRY)
        {NULL,CALL_NEWINDEX_FUNCT,0,NULL}
    };

    struct widget* const widget = (struct widget*)luaL_checkudata(L, -3, "widget_mt");

    if (lua_type(L, -2) == LUA_TSTRING)
    {
        const char* key = lua_tostring(L, -2);

        for (size_t i = 0; newindex_aa[i].key; i++)
            if (strcmp(newindex_aa[i].key, key) == 0)
            {
                if (newindex_aa[i].call_behaviour == CALL_SET_CALLBACK
                    && lua_type(L, -1) == LUA_TFUNCTION)
                {
                    int* handle = ((int*)&widget->lua.right_click) + newindex_aa[i].callback_offset;
                    *handle = luaL_ref(L, LUA_REGISTRYINDEX);
                    return 0;
                }

                if (newindex_aa[i].call_behaviour == CALL_NEWINDEX_FUNCT)
                {
                    return newindex_aa[i].funct(L);
                }
            }
    }

    if (widget->jump_table->newindex)
        return widget->jump_table->newindex(L);

    return 0;
}

/*********************************************/
/*           Widget Engine Inits             */
/*********************************************/

// Initalize the Widget Engine
void widget_engine_init()
{
    // Set empty pointers to NULL
    queue_head = NULL;
    queue_tail = NULL;

    current_drop = NULL;
    current_hover = NULL;
    last_click = NULL;

    // Arbitarly set snap to something nonzero
    snap_offset_x = 5;
    snap_offset_y = 5;  

    // Build the offscreen shader and bitmap
    offscreen_shader = al_create_shader(ALLEGRO_SHADER_GLSL);

    if (!al_attach_shader_source_file(offscreen_shader, ALLEGRO_VERTEX_SHADER, "shaders/widget.vert"))
    {
        fprintf(stderr, "Failed to attach vertex shader.\n%s\n", al_get_shader_log(offscreen_shader));
        return;
    }

    if (!al_attach_shader_source_file(offscreen_shader, ALLEGRO_PIXEL_SHADER, "shaders/widget.frag"))
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

    // Make a weak global table to contain the widgets
    // And some functions for manipulating them
    lua_newtable(main_lua_state);
    lua_newtable(main_lua_state);

    lua_pushvalue(main_lua_state, -1);
    lua_setfield(main_lua_state, -2, "__index");

    lua_pushstring(main_lua_state, "vk");
    lua_setfield(main_lua_state, -2, "__mode");

    const struct luaL_Reg widgets_methods[] = {
        {"move",widget_move_lua},
        {"lock",engine_lock},
        {"unlock",engine_unlock},
        {NULL,NULL}
    };

    luaL_setfuncs(main_lua_state, widgets_methods, 0);

    lua_setmetatable(main_lua_state, -2);
    lua_setglobal(main_lua_state, "widgets");

    // Make the widget meta table
    luaL_newmetatable(main_lua_state, "widget_mt");

    const struct luaL_Reg meta_methods[] = {
        {"__gc",gc},
        {"__newindex",newindex},
        {"__index",index},
        {NULL,NULL}
    };

    luaL_setfuncs(main_lua_state, meta_methods, 0);

    lua_pop(main_lua_state, 2);

    // Miscellaneous Lua functions

    lua_pushcfunction(main_lua_state, set_context_menu);
    lua_setglobal(main_lua_state, "contex_menu");
}

// Allocate a new widget interface and wire it into the widget engine.
// Consumes a table, if given
struct widget_interface* widget_interface_new(
    void* upcast,
    const struct widget_jump_table* jump_table)
{
    const size_t widget_size = sizeof(struct widget);

    struct widget* const widget = lua_newuserdatauv(main_lua_state, widget_size, (int)jump_table->uservalues);

    if (!widget)
        return NULL;

    *widget = (struct widget)
    {   
        .upcast = upcast,
        
        .render_interface = render_interface_new(1),

        .jump_table = jump_table,

#define CLEAR_LUA_REFNIL(method,...) .lua. ## method = LUA_REFNIL,
        FOR_CALLBACKS(CLEAR_LUA_REFNIL)     

        .next = NULL,
        .previous = queue_tail,
        .is_draggable = false,
        .is_snappable = false
    };

    // Set Metatable
    luaL_getmetatable(main_lua_state, "widget_mt");
    lua_setmetatable(main_lua_state, -2);

    lua_getglobal(main_lua_state, "widgets");
    lua_pushlightuserdata(main_lua_state, widget);
    lua_pushvalue(main_lua_state, -3);

    lua_settable(main_lua_state, -3);
    lua_pop(main_lua_state, 1);

    // If a table is avalible, treat it like a init table
    if (LUA_TTABLE == lua_type(main_lua_state, -2))
    {
        // Put the table on top and the udata widget bellow it
        lua_rotate(main_lua_state, -2, 1);

        // Process keyframes
        set_keyframe(main_lua_state);

        // Read Width
        lua_getfield(main_lua_state, -1, "width");

        if (lua_isnumber(main_lua_state, -1))
            widget->render_interface->half_width = 0.5 * luaL_checknumber(main_lua_state, -1);
  
        // Read Height
        lua_getfield(main_lua_state, -2, "height");

        if (lua_isnumber(main_lua_state, -1))
            widget->render_interface->half_height = 0.5 * luaL_checknumber(main_lua_state, -1);

        lua_pop(main_lua_state, 3);
    }

    // Wire the widget into the queue
    if (queue_tail)
        queue_tail->next = widget;
    else
    {
        queue_head = widget;
        lock = widget;
    }

    queue_tail = widget;

    return (struct widget_interface*)widget;
}
