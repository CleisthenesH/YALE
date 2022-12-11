// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include "thread_pool.h"

#include <allegro5/allegro_font.h>
#include <allegro5/allegro_opengl.h>

extern double mouse_x;
extern double mouse_y;
extern double current_timestamp;
extern const ALLEGRO_FONT* debug_font;
extern ALLEGRO_EVENT current_event;
extern const ALLEGRO_TRANSFORM identity_transform;

#include <stdio.h>
#include <float.h>
#include <math.h>

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

// Initalize the widget engine
void widget_engine_init()
{
    queue_head = NULL;
    queue_tail = NULL;

    current_drop = NULL;
    current_hover = NULL;
    last_click = NULL;

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
//  (Maybe add a second pass?)
void widget_engine_draw()
{
    glEnable(GL_STENCIL_TEST);
    style_element_setup();

    for(struct widget* widget = queue_head; widget; widget = widget->next)
	{
        const struct style_element* const style_element  = widget->style_element;
        glDisable(GL_STENCIL_TEST);
        style_element_predraw(style_element);
        call(widget, draw);

        /*
        // Not happy with this solution but it might be the best option to use the mask while keeping foiling implementation obfiscated as possible
        if (style_element->effect_flags & EFFECT_FOILED)
        {
            style_element_prefoiling(style_element);

            for (size_t i = 0; i < 4; i++)
                if (style_element->stencil_effects[i] != EFFECT_NULL)
                {
                    const int stencil_mask = 0x80 >> i;
                    glStencilFunc(GL_EQUAL, stencil_mask, stencil_mask);

                    al_set_shader_int("foiling_type", style_element->stencil_effects[i]);
                    call(widget, mask);
                }

            style_element_setup();
        }
        */
 	}

    if (0)
    {
        al_use_shader(NULL);
        al_use_transform(&identity_transform);
        al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 10, 10, ALLEGRO_ALIGN_LEFT, "%p", (void*)NULL);
    }
}

// Make a work queue with only the widgets that have an update method.
//  (Since the update method can't be changed maybe add another field for dynamic filtering,
//      since we may need filtering some times and not others and we can offload that work to the main thread)
struct work_queue* widget_engine_widget_work()
{
    struct work_queue* work_queue = work_queue_create();

    for (struct widget* widget = queue_head; widget; widget = widget->next)
        if (widget->update)
            work_queue_push(work_queue, widget->update, widget);

    return work_queue;
}

// Handle picking mouse inputs using off screen drawing.
// Will skip the current drag
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
        style_element_build_transform(widget->style_element, &transform);
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

// Update the widget engine state
void widget_engine_update()
{
    struct widget* const new_hover = widget_engine_pick(mouse_x, mouse_y);

    switch (widget_engine_state)
    {
    case ENGINE_STATE_IDLE:
    case ENGINE_STATE_HOVER:

        if (new_hover != current_hover)
        {
            if (current_hover && current_hover->hover_end)
                call(current_hover, hover_end);

            if (new_hover)
            {
                if (new_hover->hover_start)
                    call(new_hover, hover_start);

                widget_engine_state = ENGINE_STATE_HOVER;
            }
            else
                widget_engine_state = ENGINE_STATE_IDLE;

            current_hover = new_hover;
        }

        break;

    case ENGINE_STATE_CLICK_PRE_THRESHOLD:
        if (current_timestamp - left_click_timestamp > 0.2)
            if (0) // is draggable
            {
                //process drag
            }
            else
            {
                widget_engine_state = ENGINE_STATE_CLICK_POST_THRESHOLD;
            }
   

        // how to handle "slide off"??
        // ie new_hover != current hover
        break;

    case ENGINE_STATE_DRAG:
    case ENGINE_STATE_SNAP:
    case ENGINE_STATE_SNAP_TO_DRAG:
    case ENGINE_STATE_DRAG_TO_SNAP:
        // change in drop target

        break;
    }
}

// Handle events by calling all widgets that have a event handler.
// Then update the widget engine state.
//  (Thread pool the event listerner?)
void widget_engine_event_handler()
{
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
		
		}

        widget_engine_state = current_hover ? ENGINE_STATE_HOVER : ENGINE_STATE_IDLE; // Might get optimized out
        break;
    }
}

// All widgets will have a draw and mask method.
// Hence a dummy method may need to be provided
static void dummy_draw(const struct widget_interface* const widget){}

// Allocate a new widget interface and wire it into the widget engine.
struct widget_interface* widget_interface_new(
    void* upcast,
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

        .draw = draw ? draw: dummy_draw,
        .update = update,
        .event_handler = event_handler,
        .mask = mask ? mask :( draw ? draw : dummy_draw),

        .next = NULL,
        .previous = queue_tail,
    };

    if (queue_tail)
        queue_tail->next = widget;
    else
        queue_head = widget;

    queue_tail = widget;

    return (struct widget_interface*) widget;
}