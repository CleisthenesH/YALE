// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_color.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

// Turns the bitmap and lua file uploaded by Emily Huo to itch.io into a ALLEGRO_FONT
// Currently ignores kerling and xoffset
ALLEGRO_FONT* emily_huo_font(const char* font_name)
{
	char file_name_buffer[256] = "res/fonts/";

	strcat_s(file_name_buffer, 256, font_name);
	strcat_s(file_name_buffer, 256, ".png");

	ALLEGRO_BITMAP* bitmap = al_load_bitmap(file_name_buffer);
	
	strcpy_s(file_name_buffer, 256, "res/fonts/");
	strcat_s(file_name_buffer, 256, font_name);
	strcat_s(file_name_buffer, 256, ".lua");

	lua_State* lua = luaL_newstate();
	luaL_openlibs(lua);
	luaL_dofile(lua, file_name_buffer);

	if (!lua || !bitmap)
		return NULL;

	ALLEGRO_BITMAP* bitmap_font = NULL;

	int max_height = 0;
	int total_width = 1;

	// create a table to transpose the Font.char entries into being indexed by their "letter" keys
	lua_newtable(lua);

	lua_pushstring(lua, "chars");
	lua_gettable(lua, -3);

	lua_pushnil(lua);
	while (lua_next(lua, -2) != 0)
	{
		// stack: font_table, transpose_table, src_table, key, value 
		lua_pushstring(lua, "letter");
		lua_gettable(lua, -2);
		lua_pushvalue(lua, -2);
		lua_settable(lua, -6);

		// stack: font_table, transpose_table, src_table, key, value 
		lua_pushstring(lua, "width");
		lua_gettable(lua, -2);
		const int width = luaL_checkinteger(lua, -1);

		total_width += width + 1;

		lua_pop(lua, 1);

		// stack: font_table, transpose_table, src_table, key, value 
		lua_pushstring(lua, "height");
		lua_gettable(lua, -2);
		const int height = luaL_checkinteger(lua, -1);

		lua_pop(lua, 1);

		lua_pushstring(lua, "yoffset");
		lua_gettable(lua, -2);
		const int yoffset = luaL_checkinteger(lua, -1);

		if (height + yoffset > max_height)
			max_height = height + yoffset;

		lua_pop(lua, 2);
	}

	lua_pop(lua, 1);
	// stack: font_table, transposed_table

	const int spacewidth = 20;

	bitmap_font = al_create_bitmap(total_width + spacewidth, max_height + 2);

	// Directly write to bitmap_font
	al_set_target_bitmap(bitmap_font);
	al_clear_to_color(al_color_name("pink"));
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
	al_set_render_state(ALLEGRO_ALPHA_TEST, 0);

	int x, y, w, h, yoffset, xadvance;

	char buffer[2] = { 'A','\0' };

	// write space seperatly
	al_draw_filled_rectangle(1, 1, 1 + spacewidth, 1 + max_height, al_map_rgba(0, 0, 0, 0));

	xadvance = 2 + spacewidth;

	for (size_t i = 33; i <= 126; i++)
	{
		buffer[0] = (char)i;
		lua_pushstring(lua, buffer);
		lua_gettable(lua, -2);

		lua_pushstring(lua, "x");
		lua_gettable(lua, -2);
		x = luaL_checkinteger(lua, -1);

		lua_pushstring(lua, "y");
		lua_gettable(lua, -3);
		y = luaL_checkinteger(lua, -1);

		lua_pushstring(lua, "width");
		lua_gettable(lua, -4);
		w = luaL_checkinteger(lua, -1);

		lua_pushstring(lua, "height");
		lua_gettable(lua, -5);
		h = luaL_checkinteger(lua, -1);

		lua_pushstring(lua, "yoffset");
		lua_gettable(lua, -6);
		yoffset = luaL_checkinteger(lua, -1);

		al_draw_filled_rectangle(xadvance,1, xadvance + w, 1 + max_height, al_map_rgba(0, 0, 0, 0));
		al_draw_bitmap_region(bitmap, x, y, w, h, xadvance, 1+yoffset, 0);

		xadvance += w + 1; 

		lua_pop(lua, 6);
	}

	int ranges[2] = {32,126 };
	
	return al_grab_font_from_bitmap(bitmap_font, 1, ranges);
}
