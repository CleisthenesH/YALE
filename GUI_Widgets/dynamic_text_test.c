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

	al_draw_filled_circle(0, 0, 8, al_map_rgb(255, 0, 0));
	particle_bin_draw(handle->particle_bin);
}

static const struct widget_jump_table dynamic_text_test_table_entry =
{
	.draw = draw,
};

int dynamic_text_test_new(lua_State* L)
{
	struct dynamic_text_test* const handle = malloc(sizeof(struct dynamic_text_test));

	if (!handle)
		return 0;

	*handle = (struct dynamic_text_test)
	{
		.particle_bin = particle_bin_new(1),
		.widget_interface = widget_interface_new(L,handle,&dynamic_text_test_table_entry)
	};
	
	dynamic_text_new(handle->particle_bin,
		"TEST", 0, 0, 1, FONT_ID_BOLDCHEESE+1, 1, 10);

	return 1;
}
