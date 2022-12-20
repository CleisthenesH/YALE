// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_opengl.h>

#include "thread_pool.h"
extern void thread_pool_create(size_t);
extern void thread_pool_destroy();

#include "style_element.h"
extern struct work_queue* style_element_update();
extern void style_element_init();
extern void style_element_setup();

#include "widget_interface.h"
extern void widget_engine_init();
extern void widget_engine_draw();
extern struct work_queue* widget_engine_widget_work();
extern void widget_engine_update();
extern void widget_engine_event_handler();

#include "rectangle.h"
#include "card.h"

static ALLEGRO_DISPLAY* display;
static ALLEGRO_EVENT_QUEUE* main_event_queue;
struct thread_pool* thread_pool;
static bool do_exit;

// These varitables are keep seperate from their normal use so they don't change during processing  
// Also allows custom mapping on things like mouse or add a slow down factor for timestamps
double mouse_x, mouse_y;
ALLEGRO_EVENT current_event;
double current_timestamp;
double delta_time;

const ALLEGRO_TRANSFORM identity_transform;
const ALLEGRO_FONT* debug_font;

// Initalize the allegro enviroment.
static inline int allegro_init()
{
    /* Initalize the base allegro */
    if (!al_init()) {
        fprintf(stderr, "failed to initialize allegro!\n");
        return 0;
    }

    /* Initalize the drawing primitive shapes */
    if (!al_init_primitives_addon()) {
        fprintf(stderr, "failed to initialize allegro_primitives_addon!\n");
        return 0;
    }

    /* Initalize the drawing bitmaps */
    if (!al_init_image_addon()) {
        fprintf(stderr, "failed to initialize al_init_image_addon!\n");
        return 0;
    }

    /* Initalize the fonts allegro */
    if (!al_init_font_addon()) {
        fprintf(stderr, "failed to initialize al_init_font_addon!\n");
        return 0;
    }

    /* Initalize the fonts file extension allegro */
    if (!al_init_ttf_addon()) {
        fprintf(stderr, "failed to initialize al_init_ttf_addon!\n");
        return 0;
    }

    /* Initalize the keyboard allegro */
    if (!al_install_keyboard()) {
        fprintf(stderr, "failed to initialize al_install_keyboard!\n");
        return 0;
    }

    /* Initalize the mouse allegro */
    if (!al_install_mouse()) {
        fprintf(stderr, "failed to initialize al_install_mouse!\n");
        return 0;
    }


    return 1;
}

//  display
static inline void create_display()
{
    ALLEGRO_MONITOR_INFO monitor_info;
    ALLEGRO_DISPLAY_MODE display_mode;

    al_set_new_display_option(ALLEGRO_DEPTH_SIZE, 32, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_STENCIL_SIZE, 8, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_REQUIRE);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_REQUIRE);

    al_set_new_display_flags(ALLEGRO_PROGRAMMABLE_PIPELINE | ALLEGRO_OPENGL | ALLEGRO_FULLSCREEN);

    // Will improve monitor implementation when lua is itegrated
    al_get_monitor_info(0, &monitor_info);
    al_set_new_display_adapter(0);
    al_get_display_mode(al_get_num_display_modes()-1, &display_mode);

    if (0)
        display = al_create_display(
            monitor_info.x2 - monitor_info.x1,
            monitor_info.y2 - monitor_info.y1);
    else
        display = al_create_display(display_mode.width, display_mode.height);

    al_set_render_state(ALLEGRO_ALPHA_TEST, 1);
    al_set_render_state(ALLEGRO_ALPHA_FUNCTION, ALLEGRO_RENDER_NOT_EQUAL);
    al_set_render_state(ALLEGRO_ALPHA_TEST_VALUE, 0);

    al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP | ALLEGRO_NO_PRESERVE_TEXTURE);

    if (!display) {
        fprintf(stderr, "failed to create display!\n");
        return;
    }
    al_set_target_bitmap(al_get_backbuffer(display));
}

// Create event queue and register sources
static inline void create_event_queue()
{
    main_event_queue = al_create_event_queue();

    al_register_event_source(main_event_queue, al_get_display_event_source(display));
    al_register_event_source(main_event_queue, al_get_mouse_event_source());
    al_register_event_source(main_event_queue, al_get_keyboard_event_source());
}

// Initalize the global enviroment.
static inline void global_init()
{
    debug_font = al_create_builtin_font();
    al_identity_transform(&identity_transform);

    time_t time_holder;
    srand((unsigned)time(&(time_holder)));

    do_exit = false;
    current_timestamp = al_get_time();
}

// Process the current event.
static inline void process_event()
{
    switch (current_event.type)
    {
    case ALLEGRO_EVENT_DISPLAY_CLOSE:
        do_exit = true;
        return;

    case ALLEGRO_EVENT_KEY_CHAR:
        if (current_event.keyboard.modifiers & ALLEGRO_KEYMOD_CTRL &&
            current_event.keyboard.keycode == ALLEGRO_KEY_C)
        {
            do_exit = true;
            return;
        }
        break;

    case ALLEGRO_EVENT_MOUSE_AXES:
        mouse_x = current_event.mouse.x;
        mouse_y = current_event.mouse.y;

        break;
    }

    widget_engine_event_handler();
}

// On an empty queue update and draw.
static inline void empty_event_queue()
{
    // Update globals
    delta_time = al_get_time() - current_timestamp;
    current_timestamp += delta_time;

    // Update widgets
    struct work_queue* queue = style_element_update();
    thread_pool_concatenate(queue);

    queue = widget_engine_widget_work();

    thread_pool_wait();
    widget_engine_update();
    thread_pool_concatenate(queue);

    // Process predraw then wait
    al_set_target_bitmap(al_get_backbuffer(display));
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    al_reset_clipping_rectangle();

    style_element_setup();
    thread_pool_wait();

    // Draw
    widget_engine_draw(); 

    // Flip
    al_flip_display();
}

struct rectangle* test_rect;
struct keyframe destination;

static void left_click(struct widget_interface* const _)
{
    style_element_copy_destination(test_rect->widget_interface->style_element, &destination);
    style_element_interupt(test_rect->widget_interface->style_element);
}

static void right_click(struct widget_interface* const _)
{
    destination.timestamp = current_timestamp + 1;

    memcpy(style_element_new_frame(test_rect->widget_interface->style_element), &destination, sizeof(struct keyframe));
}

// Custom init during build testing.
static inline void testing_init()
{   
	 test_rect = rectangle_new();
	 {
		 struct style_element* const style_element = test_rect->widget_interface->style_element;

		 struct keyframe keyframe = (struct keyframe){ .timestamp = current_timestamp, .x = 100, .y = 100, .sx = 1, .sy = 1,.saturate = 0 };
		 style_element_set(style_element, &keyframe);

		 for (double angle = 0; angle < 6.28318530718; angle += 6.28318530718 / 1000.0)
		 {
			 *style_element_new_frame(style_element) = (struct keyframe)
			 {
				 .timestamp = current_timestamp + angle,
				 .x = 400 + 100 * cos(angle),
				 .y = 400 + 100 * sin(angle),

				 .sx = 1,
				 .sy = 1
			 };
		 }
	 }

     struct rectangle* test_rect2 = rectangle_new();
     {
         struct keyframe keyframe = (struct keyframe){ .timestamp = current_timestamp, .x = 700, .y = 100, .sx = 1, .sy = 1,.saturate = 0 };
         style_element_set(test_rect2->widget_interface->style_element, &keyframe);  

         test_rect2->widget_interface->left_click = left_click;
         test_rect2->widget_interface->right_click = right_click;
     }

    struct card* card = card_new("Alex");

    struct keyframe card_keyframe = (struct keyframe){
        .timestamp = current_timestamp, .x = 800, .y = 500, .sx = 1, .sy = 1, .saturate = 0
    };
    style_element_set(card->widget_interface->style_element, &card_keyframe);

    if(0)
    *style_element_new_frame(card->widget_interface->style_element) = (struct keyframe)
    {
        .timestamp = current_timestamp + 6, .x = 800, .y = 500, .sx = 1, .sy = 1, .saturate = 1
    };
}

// Main
int main()
{
    allegro_init();
    create_display();
    create_event_queue();
    global_init();

    thread_pool_create(8);
    style_element_init();
    widget_engine_init();

    testing_init();

    while (!do_exit)
        if (al_get_next_event(main_event_queue, &current_event))
            process_event();
        else
            empty_event_queue();    
}