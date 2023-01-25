// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include "piece_manager.h"
#include "allegro5/allegro_primitives.h"

static struct material* foil;
extern double mouse_x, mouse_y;

struct checker
{
	ALLEGRO_COLOR color;
};

static void gc(struct piece* const checker)
{

}

static void draw(const struct piece* const checker)
{
	al_draw_filled_circle(0, 0, 50, al_map_rgb_f(1, 0, 0));

	style_element_apply_material(checker->widget_interface->style_element, foil);
	material_effect_point(foil, mouse_x, mouse_y);

	al_draw_filled_circle(0, 0, 40, al_map_rgb_f(0.7, 0, 0));
}

static void mask(const struct piece* const checker)
{
	al_draw_filled_circle(0, 0, 50, al_map_rgb_f(1, 0, 0));
}

static struct piece_jump_table checker_table =
{
	.draw = draw,
	.mask = mask,
	.gc = gc,
};

void checker_new(lua_State* L)
{
	if (!foil)
	{
		foil = material_new(EFFECT_ID_RADIAL_RGB, SELECTION_ID_FULL);
	}

	piece_new(L, NULL, &checker_table);
}