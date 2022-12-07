// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>

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
extern void style_element_predraw();

static ALLEGRO_DISPLAY* display;
static ALLEGRO_EVENT_QUEUE* main_event_queue;
struct thread_pool* thread_pool;
static bool do_exit;

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

    al_set_new_display_option(ALLEGRO_DEPTH_SIZE, 32, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_STENCIL_SIZE, 8, ALLEGRO_SUGGEST);

    // Only works for primatives
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_REQUIRE);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_REQUIRE);

    if (1)
    {
        al_set_new_display_flags(ALLEGRO_PROGRAMMABLE_PIPELINE | ALLEGRO_OPENGL | ALLEGRO_FULLSCREEN);
        display = al_create_display(1920, 1080);
    }
    else
    {
        al_set_new_display_flags(ALLEGRO_PROGRAMMABLE_PIPELINE | ALLEGRO_OPENGL | ALLEGRO_WINDOWED);// | ALLEGRO_RESIZABLE);
        display = al_create_display(1920 / 2, 1080 / 2);
    }

    al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP | ALLEGRO_NO_PRESERVE_TEXTURE);

    if (!display) {
        fprintf(stderr, "failed to create display!\n");
        return 0;
    }
    al_set_target_bitmap(al_get_backbuffer(display));

    main_event_queue = al_create_event_queue();

    al_register_event_source(main_event_queue, al_get_display_event_source(display));
    al_register_event_source(main_event_queue, al_get_mouse_event_source());
    al_register_event_source(main_event_queue, al_get_keyboard_event_source());

    return 1;
}

double current_timestamp;
double delta_time;

ALLEGRO_FONT* debug_font;

// TMP varitable bellow here
struct style_element* style_element;

static inline void process_event(const ALLEGRO_EVENT* const ev)
{
    switch (ev->type)
    {
    case ALLEGRO_EVENT_DISPLAY_CLOSE:

        do_exit = true;
        return;

    case ALLEGRO_EVENT_KEY_CHAR:
        if (ev->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL)
        {
            if (ev->keyboard.keycode == ALLEGRO_KEY_C)
            {
                do_exit = true;
                return;
            }
        }
        break;
    }

    //widget_engine_event_handler(ev);
}

static inline void empty_event_queue()
{
    delta_time = al_get_time() - current_timestamp;
    current_timestamp += delta_time;

    struct work_queue* queue = style_element_update();
    thread_pool_concatenate(queue);

    // Draw
    al_set_target_bitmap(al_get_backbuffer(display));
    al_clear_to_color(al_map_rgb_f(0, 0, 0));
    al_reset_clipping_rectangle();

    style_element_predraw();
    thread_pool_wait();

    style_element_apply(style_element);
    al_draw_filled_circle(0, 0, 32, al_map_rgb(200, 0, 0));
    //al_draw_text(debug_font, al_map_rgb(0, 0, 200), 300, 300, 0, "TEST");
    //al_draw_filled_rectangle(0, 0, 1920, 1080, al_map_rgb_f(1, 1, 1));

    al_flip_display();
}

int main()
{
    allegro_init();

    thread_pool_create(8);
    style_element_init();

    style_element = style_element_new(2);

    struct keyframe keyframe = (struct keyframe){ .timestamp = al_get_time(), .x = 0, .y = 0, .sx = 1, .sy = 1,.saturate = 0 };
    style_element_set(style_element, &keyframe);
    
    struct keyframe* keyframe2 = style_element_new_frame(style_element);    
    //*keyframe2 = (struct keyframe){ .timestamp = al_get_time()+2, .x = 500, .y = 500, .sx = 1, .sy = 1, .saturate = 1 };
    *keyframe2 = (struct keyframe){ .timestamp = al_get_time()+2, .x = 0, .y = 0, .sx = 1, .sy = 1, .saturate = 0 };


    debug_font = al_create_builtin_font();

    {
        time_t time_holder;
        srand((unsigned)time(&(time_holder)));
    }

    // ALL THESE THINGS ARE OCCUPING THE STACK
    ALLEGRO_EVENT* const ev = (ALLEGRO_EVENT*)malloc(sizeof(ALLEGRO_EVENT));

    do_exit = false;
    current_timestamp = al_get_time();

    if (!ev)
        return 0;

    while (!do_exit)
        if (al_get_next_event(main_event_queue, ev))
            process_event(ev);
        else
            empty_event_queue();
}