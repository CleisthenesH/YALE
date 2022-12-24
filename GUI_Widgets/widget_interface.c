// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include "thread_pool.h"

#include <allegro5/allegro_font.h>
#include <allegro5/allegro_opengl.h>

#include <stdio.h>
#include <float.h>
#include <math.h>

extern void style_element_setup();
extern void style_element_predraw(const struct style_element* const);

extern double mouse_x;
extern double mouse_y;
extern double current_timestamp;
extern const ALLEGRO_FONT* debug_font;
extern ALLEGRO_EVENT current_event;
extern const ALLEGRO_TRANSFORM identity_transform;

struct widget
{
	struct widget_interface;

    struct widget* next;
    struct widget* previous;

    void (*draw)(const struct widget_interface* const);
    void (*update)(struct widget_interface* const);
    void (*event_handler)(struct widget_interface* const);
    void (*mask)(const struct widget_interface* const);
};

#define call(widget,method) (widget)->method((struct widget_interface*) (widget))
#define call_va(widget,method,...) (widget)->method((struct widget_interface*) (widget),##__VA_ARG__)

static enum {
    ENGINE_STATE_IDLE,                      // Idle state
    ENGINE_STATE_HOVER,                     // A widget is being hovered
    ENGINE_STATE_CLICK_PRE_THRESHOLD,       // Inbetween mouse down and callback (checking if click or drag)
    // Change to PRE_DRAG_THRESHOLD?
    ENGINE_STATE_CLICK_POST_THRESHOLD,      // We are no longer waiting but, but the widget can't be dragged
    ENGINE_STATE_DRAG,                      // A widget is getting dragged
    ENGINE_STATE_SNAP_TO_DRAG,
    ENGINE_STATE_DRAG_TO_SNAP,
    ENGINE_STATE_SNAP
} widget_engine_state;

static const char* engine_state_str[] = {
       "Idle",
       "Hover",
       "Click (Pre-Threshold)",
       "Click (Post-Threshold)",
       "Drag",
       "Snap to Drag",
       "Drag to Sanp",
       "Snap"
};

static struct widget* queue_head;
static struct widget* queue_tail;

static ALLEGRO_SHADER* offscreen_shader;
static ALLEGRO_BITMAP* offscreen_bitmap;

static double left_click_timestamp;

static struct widget* last_click;
static struct widget* current_hover;
static struct widget* current_drop;

static struct keyframe drag_release;
static double drag_offset_x, drag_offset_y;
static double snap_offset_x, snap_offset_y;

// Initalize the widget engine
void widget_engine_init()
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
}

// Draw the widgets in queue order.
void widget_engine_draw()
{
//  (Maybe add a second pass?)
    style_element_setup();

    for(struct widget* widget = queue_head; widget; widget = widget->next)
	{
        const struct style_element* const style_element  = widget->style_element;
        style_element_predraw(style_element);
        call(widget, draw);
 	}

    if (1)
    {
        al_use_shader(NULL);
        glDisable(GL_STENCIL_TEST);
        al_use_transform(&identity_transform);

        al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 10, 10, ALLEGRO_ALIGN_LEFT, "Hover: %p", current_hover);
        al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 10, 40, ALLEGRO_ALIGN_LEFT, "Drop: %p", current_drop);
        al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 10, 70, ALLEGRO_ALIGN_LEFT, "Drag Release: %f %f", drag_release.x, drag_release.y);
    }
}

// Make a work queue with only the widgets that have an update method.
struct work_queue* widget_engine_widget_work()
{
 //  (Since the update method can't be changed maybe add another field for dynamic filtering,
//      since we may need filtering some times and not others and we can offload that work to the main thread)
    struct work_queue* work_queue = work_queue_create();

    for (struct widget* widget = queue_head; widget; widget = widget->next)
        if (widget->update)
            work_queue_push(work_queue, widget->update, widget);

    return work_queue;
}

// Handle picking mouse inputs using off screen drawing.
static inline struct widget* widget_engine_pick(int x, int y)
{
    // Will skip the current drag
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
            widget_engine_state == ENGINE_STATE_DRAG_TO_SNAP ||
            widget_engine_state == ENGINE_STATE_SNAP_TO_DRAG
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
        call(widget, mask);
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

static inline double distance(struct widget_interface* widget, double x, double y)
{
    const double dx = widget->style_element->current.x - x;
    const double dy = widget->style_element->current.y - y;

    return sqrtf(dx * dx + dy * dy);
}

// Updates current_hover keyframe keyframes to move towards the drag
static inline void widget_set_drag()
{
    style_element_interupt(current_hover->style_element);

    const double dx = current_hover->style_element->current.x - (drag_offset_x + mouse_x);
    const double dy = current_hover->style_element->current.y - (drag_offset_y + mouse_y);
    const double distance = sqrtf(dx * dx + dy * dy);
    const double speed = 1000; //px per sec

    if (distance > 0.001)
    {
        struct keyframe* keyframe = style_element_new_frame(current_hover->style_element);

        keyframe->x = drag_offset_x + mouse_x;
        keyframe->y = drag_offset_y + mouse_y;
        keyframe->timestamp = current_timestamp + distance / speed;
    }
    else
    {
        current_hover->style_element->current.x = drag_offset_x + mouse_x;
        current_hover->style_element->current.y = drag_offset_y + mouse_y;
    }
}

// Return current_drag to drag_release
static inline void widget_drag_release()
{
    style_element_interupt(current_hover->style_element);

    const double dx = current_hover->style_element->current.x - drag_release.x;
    const double dy = current_hover->style_element->current.y - drag_release.y;
    const double distance = sqrtf(dx * dx + dy * dy);
    const double speed = 1000; //px per sec

	struct keyframe* keyframe = style_element_new_frame(current_hover->style_element);

	memcpy(keyframe,&drag_release,sizeof(struct keyframe));
    keyframe->timestamp = al_current_time() + distance/ speed;
}

static inline void widget_engine_update_drag_pointers()
{
    struct widget* const new_pointer = widget_engine_pick(mouse_x, mouse_y);

    if ((widget_engine_state == ENGINE_STATE_IDLE ||
        widget_engine_state == ENGINE_STATE_HOVER) &&
        current_hover != new_pointer)
    {
        if (current_hover && current_hover->hover_end)
            call(current_hover, hover_end);

        if (new_pointer)
        {
            if (new_pointer->hover_start)
                call(new_pointer, hover_start);

            widget_engine_state = ENGINE_STATE_HOVER;
        }
        else
            widget_engine_state = ENGINE_STATE_IDLE;

        current_hover = new_pointer;
    }

    if (current_drop != new_pointer)
    {
        switch (widget_engine_state)
        {
        case ENGINE_STATE_DRAG:
        case ENGINE_STATE_SNAP:
        case ENGINE_STATE_SNAP_TO_DRAG:
        case ENGINE_STATE_DRAG_TO_SNAP:

            if (current_drop && current_drop->drop_end)
                call(current_drop, drop_end);

            if (new_pointer)
            {
                if (new_pointer->drop_start)
                    call(new_pointer, drop_start);

                if (new_pointer->is_snappable)
                {
                    style_element_interupt(current_hover->style_element);
                    struct keyframe* snap_target = style_element_new_frame(current_hover->style_element);

                    snap_target->x = new_pointer->style_element->current.x + snap_offset_x;
                    snap_target->y = new_pointer->style_element->current.y + snap_offset_y;

                    const double dx = current_hover->style_element->current.x - snap_target->x;
                    const double dy = current_hover->style_element->current.y - snap_target->y;
                    const double distance = sqrtf(dx * dx + dy * dy);
                    const double speed = 1000; //px per sec

                    snap_target->timestamp = current_timestamp + distance/speed;

                    widget_engine_state = ENGINE_STATE_DRAG_TO_SNAP;
                }
            }
            else
            {
                widget_set_drag();
                widget_engine_state = current_drop && current_drop->is_snappable? ENGINE_STATE_SNAP_TO_DRAG : ENGINE_STATE_DRAG;
            }

            break;
        }

        current_drop = new_pointer;
    }
}

// Update the widget engine state
void widget_engine_update()
{
    widget_engine_update_drag_pointers();

    // Update drag position
    switch (widget_engine_state)
    {
    case ENGINE_STATE_CLICK_PRE_THRESHOLD:
        if (current_timestamp - left_click_timestamp > 0.2)
			if (current_hover->is_draggable) // is draggable
			{
				if (current_hover->drag_start)
					call(current_hover, drag_start);

				widget_engine_state = ENGINE_STATE_DRAG;
			}
			else
			{
				widget_engine_state = ENGINE_STATE_CLICK_POST_THRESHOLD;
			}
        break;

    case ENGINE_STATE_DRAG:
        current_hover->style_element->current.x = drag_offset_x + mouse_x;
        current_hover->style_element->current.y = drag_offset_y + mouse_y;
        break;

    case ENGINE_STATE_SNAP_TO_DRAG:
        widget_set_drag();
        //widget_engine_state = ENGINE_STATE_DRAG;

        break;
    }
}

// Handle events by calling all widgets that have a event handler.
void widget_engine_event_handler()
{
    // Then update the widget engine state.
//  (Thread pool the event listerner?)

    for (struct widget* widget = queue_head; widget; widget = widget->next)
        if (widget->event_handler)
            call(widget, event_handler);

    switch (current_event.type)
    {
    case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
        if (!current_hover)
            break;

        if (current_event.mouse.button == 2)
        {
            if (current_hover->right_click)
                call(current_hover, right_click);
        }

        if (current_event.mouse.button == 1)
        {
            left_click_timestamp = current_timestamp;
            widget_engine_state = ENGINE_STATE_CLICK_PRE_THRESHOLD;

            drag_offset_x = current_hover->style_element->current.x - mouse_x;
            drag_offset_y = current_hover->style_element->current.y - mouse_y;
            style_element_copy_destination(current_hover->style_element, &drag_release);

            if (current_hover != last_click)
            {
                if (last_click && last_click->click_off)
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
        case ENGINE_STATE_CLICK_PRE_THRESHOLD:
            if (current_hover->left_click)
            {
                call(current_hover, left_click);
            }
            break;

        case ENGINE_STATE_DRAG:
        case ENGINE_STATE_SNAP:
        case ENGINE_STATE_DRAG_TO_SNAP:
        case ENGINE_STATE_SNAP_TO_DRAG:
            // if(current_drop)
            if (current_hover->drag_end_no_drop)
                call(current_hover, drag_end_no_drop);

            widget_drag_release();
            //style_element_set(current_hover->style_element, &drag_release);
        }

        widget_engine_state = current_hover ? ENGINE_STATE_HOVER : ENGINE_STATE_IDLE; // Might get optimized out
        break;
    }
}

// All widgets will have a draw and mask method.
static void dummy_draw(const struct widget_interface* const widget){}

// Allocate a new widget interface and wire it into the widget engine.
struct widget_interface* widget_interface_new(
    const void* const upcast,
    void (*draw)(const struct widget_interface* const),
    void (*update)(struct widget_interface* const),
    void (*event_handler)(struct widget_interface* const),
    void (*mask)(const struct widget_interface* const))
{
    struct widget* widget = malloc(sizeof(struct widget));

    if (!widget)
        return NULL;

    *widget = (struct widget)
    {
        .upcast = upcast,
        .style_element = style_element_new(1),

        .draw = draw ? draw : dummy_draw,
        .update = update,
        .event_handler = event_handler,
        .mask = mask ? mask : (draw ? draw : dummy_draw),

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

    return (struct widget_interface*) widget;
}