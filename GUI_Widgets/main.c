// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_opengl.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include "thread_pool.h"
extern void thread_pool_init(size_t);
extern void thread_pool_destroy();

#include "style_element.h"
extern struct work_queue* style_element_update();
extern struct work_queue* tweener_update();
extern void style_element_init();
extern void style_element_setup();

#include "widget_interface.h"
extern void widget_engine_init(lua_State*);
extern void widget_engine_draw();
extern struct work_queue* widget_engine_widget_work();
extern void widget_engine_update();
extern void widget_engine_event_handler();
extern void widget_style_sheet_init();

extern void piece_manager_setglobal(lua_State*);

#include "resource_manager.h"
void resource_manager_init();

static ALLEGRO_DISPLAY* display;
static ALLEGRO_EVENT_QUEUE* main_event_queue;
struct thread_pool* thread_pool;
static bool do_exit;

// These varitables are keep seperate from their normal use so they don't change during processing  
// Also allows custom mapping on things like mouse or add a slow down factor for timestamps
double mouse_x, mouse_y;
ALLEGRO_EVENT current_event;
double current_timestamp;
double delta_timestamp;

const ALLEGRO_TRANSFORM identity_transform;
const ALLEGRO_FONT* debug_font; 

const lua_State* main_lua_state;

// Initalze the lua enviroment.
static inline int lua_init()
{
    main_lua_state = luaL_newstate();

    if (!main_lua_state)
        return 0;

    luaL_openlibs(main_lua_state);
    piece_manager_setglobal(main_lua_state);

    return 1;
}

// Config and create display
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
    al_get_display_mode(al_get_num_display_modes() - 1, &display_mode);

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

    create_display();
    create_event_queue();

    return 1;
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
    delta_timestamp = al_get_time() - current_timestamp;
    current_timestamp += delta_timestamp;

    // Update widgets
    struct work_queue* queue = style_element_update();
    thread_pool_concatenate(queue);

    queue = widget_engine_widget_work();

    thread_pool_wait();
    widget_engine_update();
    thread_pool_concatenate(queue);

    // Process predraw then wait
    al_set_target_bitmap(al_get_backbuffer(display));
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
    //al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
    al_set_render_state(ALLEGRO_ALPHA_TEST, 1);

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

// Error call   
static inline void main_state_dofile(const char* file_name)
{
    int error = luaL_dofile(main_lua_state, file_name);

    if (error == LUA_OK)
        return;

    printf("While running file % s an error of type: \"", file_name);

    switch (error)
    {
    case LUA_ERRRUN:
        printf("Runtime");
        break;
    case LUA_ERRMEM:
        printf("Memmory Allocation");
        break;
    case LUA_ERRERR:
        printf("Message Handler");
        break;
    case LUA_ERRSYNTAX:
        printf("Syntax");
        break;
    case LUA_YIELD:
        printf("Unexpected Yeild");
        break;
    case LUA_ERRFILE:
        printf("File Related");
        break;
    default:
        printf("Unkown Type (%d)", error);
    }

    const char* error_message = luaL_checkstring(main_lua_state, -1);

    printf("\" occurred\n\t%s\n\n", error_message);
}

// Main
int main()
{
    lua_init();

    main_state_dofile("boot.lua");

    allegro_init();
    resource_manager_init();
    global_init();
    thread_pool_init(8);
    style_element_init();
    widget_style_sheet_init();
    widget_engine_init(main_lua_state);

    main_state_dofile("post_boot.lua");

    while (!do_exit)
        if (al_get_next_event(main_event_queue, &current_event))
            process_event();
        else
            empty_event_queue();    
}