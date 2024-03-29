// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// Some quick and easy compile options
#define EASY_BACKGROUND
// #define EASY_FPS

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


// Thread Pool includes
#include "thread_pool.h"
void thread_pool_init(size_t);
void thread_pool_destroy();

// Renderer includes
struct work_queue* render_interface_update();
void render_interface_init();
void render_interface_global_predraw();

void tweener_init();
struct work_queue* tweener_update();

void particle_engine_init();
struct work_queue* particle_engine_update();

// Widget Interface includes
void widget_engine_init(lua_State*);
void widget_engine_draw();
struct work_queue* widget_engine_widget_work();
void widget_engine_update();
void widget_engine_event_handler();
void widget_style_sheet_init();

void board_manager_init(lua_State*);

// Resource Manager includes
void resource_manager_init();

// Scheduler includes
ALLEGRO_EVENT_SOURCE* scheduler_init();
void scheduler_generate_events();

// Miscellaneous Lua Interfaces
void lua_openL_misc(lua_State*);

// Static variable declaration
static ALLEGRO_DISPLAY* display;
static ALLEGRO_EVENT_QUEUE* main_event_queue;
struct thread_pool* thread_pool;
static bool do_exit;

// A simple background for testing
#ifdef EASY_BACKGROUND
static ALLEGRO_BITMAP* easy_background;
const double easy_background_speed = 0;
#include "material.h"
#endif

// A simple FPS Monitor
#ifdef EASY_FPS
static double last_render_timestamp;
#endif

// These varitables are keep seperate from their normal use so they don't change during processing  
// Also allows custom mapping on things like mouse or add a slow down factor for timestamps
double mouse_x, mouse_y;
ALLEGRO_EVENT current_event;
double future_timestamp;
double current_timestamp;
double residual_timestamp;
double delta_timestamp;

// TODO: find a way to make debug_font's type "const ALLEGRO_FONT* const"
//          main_lua_state's type "lua_State* const"
//          identity_transform type "const ALLEGRO_TRANSFORM
ALLEGRO_TRANSFORM identity_transform;
ALLEGRO_FONT* debug_font; 
lua_State* main_lua_state;

// Initalze the lua enviroment.
static inline int lua_init()
{
    main_lua_state = luaL_newstate();

    if (!main_lua_state)
        return 0;

    luaL_openlibs(main_lua_state);
    board_manager_init(main_lua_state);

    return 1;
}

// Config and create display
static inline void create_display()
{
    ALLEGRO_MONITOR_INFO monitor_info;
    ALLEGRO_DISPLAY_MODE display_mode;

    al_set_new_display_flags(ALLEGRO_PROGRAMMABLE_PIPELINE | ALLEGRO_OPENGL | ALLEGRO_FULLSCREEN);

    al_set_new_display_option(ALLEGRO_DEPTH_SIZE, 32, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_STENCIL_SIZE, 8, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_REQUIRE);
    al_set_new_display_option(ALLEGRO_SAMPLES, 16, ALLEGRO_REQUIRE);

#define ALLEGRO_BILINEAR_FILTER ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR
#define ALLEGRO_TRILINEAR_FILTER ALLEGRO_BILINEAR_FILTER | ALLEGRO_MIPMAP

    // One makes the text look nice the other the icons.
    // al_set_new_bitmap_samples
    if(1)
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR | ALLEGRO_VIDEO_BITMAP | ALLEGRO_NO_PRESERVE_TEXTURE);
    else
    al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP | ALLEGRO_NO_PRESERVE_TEXTURE);

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

    if (!display) {
        fprintf(stderr, "failed to create display!\n");
        return;
    }

#ifdef EASY_BACKGROUND
    easy_background = al_create_bitmap(al_get_display_width(display) + 100, al_get_display_height(display) + 100);

    al_set_target_bitmap(easy_background);

    // Keeping these here incase I change init call order
    // 
    //al_use_transform(&identity_transform);
    //al_use_shader(NULL);

    al_clear_to_color(al_color_name("aliceblue"));
    unsigned int i, j;

    const unsigned int height = al_get_display_height(display) / 10 + 10;
    const unsigned int width = al_get_display_width(display) / 10 + 10;

	for (i = 0; i <= width; i++)
		for (j = 0; j <= height; j++)
			if (i % 10 == 0 && j % 10 == 0)
				al_draw_circle(10 * i, 10 * j, 2, al_color_name("darkgray"), 2);
			else
				al_draw_circle(10 * i, 10 * j, 1, al_color_name("grey"), 0);

#endif

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
    future_timestamp = current_timestamp;
    residual_timestamp = 0;
    delta_timestamp = 0;

#ifdef EASY_FPS
    last_render_timestamp = current_timestamp;
#endif
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

    case ALLEGRO_GET_EVENT_TYPE('T', 'I', 'M', 'E'):
        ((void (*)(void*)) current_event.user.data1)((void*) current_event.user.data2);

        return;

    }

    widget_engine_event_handler();
}

// On an update and also draw, depending on flag
static inline void update_and_draw(const bool do_draw)
{
    // Updating and drawing are combined into one function to help weaving in the thread pool.
    // With a const bool and inline function the compiler should figure it out
     
    // future_timestamp and current_time_stamp must be accurate upon calling this function.
    delta_timestamp = future_timestamp - current_timestamp;

    // Update tweeners
    struct work_queue* queue = tweener_update();
    thread_pool_concatenate(queue);

    // Update render_interfaces
    // (Currently only copies changes from keyframe tweeners across.)
    queue = render_interface_update();
    thread_pool_concatenate(queue);

    // Update the particle engine
    queue = particle_engine_update();
    thread_pool_concatenate(queue);

    // Create the widget work queue, but do not concatinate to the thread pool.
    queue = widget_engine_widget_work();
    thread_pool_wait();

    // Do a global widget_engine update then concatinate the widget work to the threadpool.
    widget_engine_update();
    thread_pool_concatenate(queue);

    current_timestamp = future_timestamp;

    // If we arn't drawing 
    if (!do_draw)
    {
        thread_pool_wait();
        return;
    }

    // The residual time can be used for projection in drawing
    residual_timestamp = al_get_time() - future_timestamp;

    // Process predraw then wait
    al_set_target_bitmap(al_get_backbuffer(display));
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
    al_set_render_state(ALLEGRO_ALPHA_TEST, 1);

    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    al_reset_clipping_rectangle();

    render_interface_global_predraw();
 
#ifdef EASY_BACKGROUND
    al_use_transform(&identity_transform);
    material_apply( NULL);
    const double easy_background_offset = -fmod(easy_background_speed * current_timestamp, 100);
    al_draw_bitmap(easy_background, easy_background_offset, easy_background_offset, 0);
#endif

    thread_pool_wait();

    // Draw
    widget_engine_draw();

#ifdef EASY_FPS
    al_use_transform(&identity_transform);
    material_apply(NULL);
    al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 0, 0, 0, "FPS:%lf  Timestamp:%lf",1.0/(current_timestamp-last_render_timestamp), current_timestamp);
    last_render_timestamp = current_timestamp;
#endif

    // Flip
    al_flip_display();
}

// Wraps the lua_dofile with some error reporting
static inline int lua_dofile_wrapper(const char* file_name)
{
    int error = luaL_dofile(main_lua_state, file_name);

    if (error == LUA_OK)
        return LUA_OK;

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

    return error;
}

// Resolve and Run a bootfile based on main_lua_state
static inline void lua_boot_file()
{
    lua_getglobal(main_lua_state, "boot_file");

    if (lua_isstring(main_lua_state, -1))
    {
        const char* boot_file = luaL_checkstring(main_lua_state, -1);

        lua_dofile_wrapper(boot_file);

        lua_pushnil(main_lua_state);
        lua_setglobal(main_lua_state, "boot_file");
    }
    else
    {
        lua_dofile_wrapper("boot.lua");
    }

    lua_pop(main_lua_state, 1);
}

/* System Dependency :
*       |-------------(Only config.lua )-------------> ALLEGRO
*       |                                                |
*       |         _______________________________________|____________________
*       |         V             V                     |           |           |
*       |-> (Scheduler) <- (Thread Pool)              |           |           V
*  LUA  |                    V       V                |           |   (Resource Manager)
*       |             (Tweener)    (Particle)         |           |
*       |                    V                        V           |
*       |-----------------> (     Render     ) <---- (Material)   |
*       |                     V          |                        |
*       |--------------->  (Camera)      |                        |
*       |                                V                        V
*       |---------------------> (            Widget Interface         )
*/

// Main
int main()
{
    // Init Lua first so we can read a config file to inform later inits
    lua_init();
    lua_dofile_wrapper("config.lua");

    // Init the Allegro Environment
    allegro_init();
    thread_pool_init(8);
    global_init();

    // Init Systems, check dependency graph for order.
    resource_manager_init();
    al_register_event_source(main_event_queue,scheduler_init());
    tweener_init();
    particle_engine_init();
    render_interface_init();

    // Miscellaneous Lua Interfaces
    lua_openL_misc(main_lua_state);

    // Init Widgets
    widget_style_sheet_init();
    widget_engine_init(main_lua_state);

    // Resolve and Read Boot File
    lua_boot_file();

    // Main loop
    while (!do_exit)
    {
        future_timestamp = al_get_time();

        if (future_timestamp - current_timestamp > 0.25)
            future_timestamp = current_timestamp + 0.25;

        scheduler_generate_events();

        if (al_peek_next_event(main_event_queue, &current_event)
            && current_event.any.timestamp <= future_timestamp)
        {
            future_timestamp = current_event.any.timestamp;

            update_and_draw(false);
            process_event();
            al_drop_next_event(main_event_queue);

            continue;
        }


        update_and_draw(true);
    }
}
