// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "pieces_manager.h"
#include "allegro5/allegro_primitives.h"

struct square
{
	ALLEGRO_COLOR color;
};

static void gc(struct zone* const square)
{

}

static void draw(const struct zone* const square)
{
	if(square->nominated)
		al_draw_filled_rectangle(-50, -50, 50, 50, al_map_rgb_f(1, 1, 0));
	else 
		if (square->highlighted)
			al_draw_filled_rectangle(-50, -50, 50, 50, al_map_rgb(255, 105, 180));
		else
			al_draw_filled_rectangle(-50, -50, 50, 50, al_map_rgb_f(1, 1, 1));
}

static void mask(const struct zone* const square)
{
	al_draw_filled_rectangle(-50, -50, 50, 50, al_map_rgb_f(1, 1, 1));
}

static struct zone_jump_table square_table =
{
	.draw = draw,
	.mask = mask,
	.gc = gc,
};

struct zone* square_new(lua_State* L)
{
	return zone_new(L, NULL, &square_table);
}