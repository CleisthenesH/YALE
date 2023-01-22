// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "piece_manager.h"
#include "allegro5/allegro_primitives.h"

struct square
{
	ALLEGRO_COLOR color;
};

static void gc(struct game_zone* const square)
{

}

static void draw(const struct game_zone* const square)
{
	al_draw_filled_rectangle(-50, -50, 50, 50, al_map_rgb_f(1, 1, 1));
}

static void mask(const struct game_zone* const square)
{
	al_draw_filled_rectangle(-50, -50, 50, 50, al_map_rgb_f(1, 1, 1));
}

static struct game_zone_jump_table square_table =
{
	.draw = draw,
	.mask = mask,
	.gc = gc,
};

void square_new(lua_State* L)
{
	zone_new(L, NULL, &square_table);
}