// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// A barebones widget for prototyping

#include "widget_interface.h"
#include "dynamic_text.h"

#include <allegro5/allegro_primitives.h>

struct dynamic_text_test
{
	struct widget_interface* widget_interface;
	struct particle_bin* particle_bin;
};



static void draw(const struct widget_interface* const widget)
{
	struct dynamic_text_test* const handle = (struct dynamic_text_test* const)widget->upcast;

	al_draw_filled_circle(0, 0, 2, al_map_rgb(255, 0, 0));
	particle_bin_draw(handle->particle_bin);
}

static const struct widget_jump_table dynamic_text_test_table_entry =
{
	.draw = draw,
};

int dynamic_text_test_new(lua_State* L)
{
	struct dynamic_text_test* const handle = malloc(sizeof(struct dynamic_text_test));

	size_t font_id = 0;
	size_t animation_id = 0;

	if (lua_istable(L, -1))
	{
		lua_getfield(L, -1, "font");

		if (lua_isinteger(L, -1))
			font_id = luaL_checkinteger(L, -1);

		if (font_id > FONT_ID_COUNT)
			font_id = 0;

		// Pool party font is bugged
		if (font_id == 17)
			font_id = 0;

		lua_getfield(L, -2, "animation");

		if (lua_isinteger(L, -1))
			animation_id = luaL_checkinteger(L, -1);

		if (animation_id > DYNAMIC_TEXT_MAX)
			animation_id = 0;

		lua_pop(L, 2);
	}

	if (!handle)
		return 0;

	*handle = (struct dynamic_text_test)
	{
		.particle_bin = particle_bin_new(1),
		.widget_interface = widget_interface_new(L,handle,&dynamic_text_test_table_entry)
	};

	dynamic_text_new(handle->particle_bin,
		"TEST", 0, 0, 1, font_id, animation_id, 100);

	return 1;
}
