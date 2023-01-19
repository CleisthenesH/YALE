// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "piece_manager.h"
#include "allegro5/allegro_primitives.h"

struct checker
{
	ALLEGRO_COLOR color;
};

static void gc(struct game_piece* const checker)
{

}

static void draw(const struct game_piece* const checker)
{
	al_draw_filled_circle(0, 0, 50, al_map_rgb_f(1, 0, 0));
	al_draw_filled_circle(0, 0, 40, al_map_rgb_f(0.7, 0, 0));
}

static void mask(const struct game_piece* const checker)
{
	al_draw_filled_circle(0, 0, 50, al_map_rgb_f(1, 0, 0));
}

static struct game_piece_jump_table checker_table =
{
	.draw = draw,
	.mask = mask,
	.gc = gc,
};

void checker_new(lua_State* L)
{
	piece_new(L, NULL, &checker_table);
}