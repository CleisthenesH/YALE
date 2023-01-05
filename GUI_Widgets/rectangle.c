// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "rectangle.h"
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_opengl.h>

static const struct widget_jump_table rectangle_jump_table_entry;

extern ALLEGRO_FONT* debug_font;

static void draw(const struct widget_interface* const widget)
{
	const struct rectangle* const rectangle = (struct rectangle*) widget->upcast;
	const double half_width = 0.5*rectangle->width;
	const double half_height = 0.5*rectangle->height;

	//glStencilMask(0x80);
	//glStencilFunc(GL_REPLACE, 0x80, 0x80);

	style_element_effect(NULL, NULL);
	al_draw_filled_rectangle(-half_width, -half_height, half_width, half_height, rectangle->color);
	al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 0, 0, ALLEGRO_ALIGN_CENTER, "%d", rectangle->cnt);
}

static void hover_start(struct widget_interface* const widget)
{
	struct rectangle* const rectangle = (struct rectangle*)widget->upcast;

	rectangle->color = al_map_rgb_f(1, 0, 0);
}

static void hover_end(struct widget_interface* const widget)
{
	struct rectangle* const rectangle = (struct rectangle*)widget->upcast;

	rectangle->color = al_map_rgb_f(1, 1,1);
}

static void left_click(struct widget_interface* const widget)
{
	struct rectangle* const rectangle = (struct rectangle*)widget->upcast;

	rectangle->cnt++;
}

static void right_click(struct widget_interface* const widget)
{
	struct rectangle* const rectangle = (struct rectangle*)widget->upcast;

	rectangle->cnt--;
}

static void gc(void* rectangle)
{
	printf("gc hit\n");
}

static void read_color(lua_State* L, ALLEGRO_COLOR* color)
{
	// need no make it return a flag
	// or maybe optimize for tail call?

	luaL_checktype(L, -1, LUA_TTABLE);

	lua_geti(L, -1, 1);

	if (!lua_isnil(L, -1))
	{
		lua_geti(L, -2, 1);
		lua_geti(L, -3, 1);

		const int r = lua_tointeger(L, -3);
		const int g = lua_tointeger(L, -2);
		const int b = lua_tointeger(L, -1);

		lua_settop(L, -4);

		*color = al_map_rgb(r, g, b);
		return;
	}

	lua_pushstring(L, "r");
	lua_gettable(L, -3);

	if (!lua_isnil(L, -1))
	{		
		lua_pushstring(L, "g");
		lua_gettable(L, -4);
		lua_pushstring(L, "b");
		lua_gettable(L, -5);

		const int r = lua_tointeger(L, -3);
		const int g = lua_tointeger(L, -2);
		const int b = lua_tointeger(L, -1);

		lua_settop(L, -5);

		*color = al_map_rgb(r, g, b);
		return;
	}

	lua_settop(L, -2);

	return;
}

static void write_color(lua_State* L, ALLEGRO_COLOR* color)
{
	lua_createtable(L, 3, 3);

	lua_pushinteger(L, color->r*255);
	lua_seti(L, -2, 1);
	lua_pushinteger(L, color->g*255);
	lua_seti(L, -2, 2);
	lua_pushinteger(L, color->b*255);
	lua_seti(L, -2, 3);

	lua_pushstring(L, "r");
	lua_pushinteger(L, color->r * 255);
	lua_settable(L, -3);
	lua_pushstring(L, "g");
	lua_pushinteger(L, color->g*255);
	lua_settable(L, -3);
	lua_pushstring(L, "b");
	lua_pushinteger(L, color->b*255);
	lua_settable(L, -3);

	return;
}

static int newindex(lua_State* L)
{
	struct widget_interface* const widget = check_widget(L, -3, &rectangle_jump_table_entry);
	struct rectangle* const rectangle = widget->upcast;

	if (!widget)
		return 0;

	if (lua_type(L, -2) == LUA_TSTRING)
	{
		const char* key = lua_tostring(L, -2);

		if (strcmp(key, "color") == 0)
		{
			read_color(L, &rectangle->color);

			return 0;
		}
	}

	return 0;
}

static int index(lua_State* L)
{
	struct widget_interface* const widget = check_widget(L, -2, &rectangle_jump_table_entry);
	struct rectangle* const rectangle = widget->upcast;

	if (!widget)
		return 0;

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = lua_tostring(L, -1);

		if (strcmp(key, "color") == 0)
		{
			write_color(L, &rectangle->color);

			return 1;
		}
	}

	return 0;
}

static const struct widget_jump_table rectangle_jump_table_entry = 
{
	.gc = gc,
	.draw = draw,
	.mask = draw,
	.hover_start = hover_start,
	.hover_end = hover_end,
	.left_click = left_click,
	.right_click = right_click,
	.newindex = newindex,
	.index = index
};

int rectangle_new(lua_State* L)
{
	struct rectangle* const rectangle = malloc(sizeof(struct rectangle));

	if (!rectangle)
		return 0;

	*rectangle = (struct rectangle)
	{
		.color = al_map_rgb_f(1,1,1),
		.height = 32,
		.width = 100,
		.cnt = 0,
		.widget_interface = widget_interface_new(L,rectangle,&rectangle_jump_table_entry)
	};

	return 1;
}

