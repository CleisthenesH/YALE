// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include "pieces_manager.h"
#include "allegro5/allegro_primitives.h"
#include "material.h"

#include <math.h>

static struct material* foil;
extern double mouse_x, mouse_y;

#include "resource_manager.h"

struct checker
{
	ALLEGRO_COLOR color;
};

static void particle(double timestamp, size_t seed)
{
	const double theta =  timestamp;
	al_draw_filled_circle(100*sinf(theta), 100*cosf(theta), 10, al_map_rgb_f(0, 0, 1));
}

static void gc(struct piece* const checker)
{

}

static void draw(const struct piece* const checker)
{
	al_draw_filled_circle(0, 0, 50, al_map_rgb_f(1, 0, 0));

	material_point(foil, mouse_x, mouse_y);
	material_apply(foil);

	al_draw_filled_circle(0, 0, 40, al_map_rgb_f(0.7, 0, 0));

	material_apply(NULL);

	//style_element_draw_particles(checker->widget_interface->style_element);

	al_draw_bitmap(resource_manager_icon(ICON_ID_ACCORDION),0,0,0);

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

struct piece* checker_new(lua_State* L)
{
	if (!foil)
	{
		foil = material_new(MATERIAL_ID_RADIAL_RGB, SELECTION_ID_FULL);
	}

	struct piece* piece = piece_new(L, NULL, &checker_table);

	//style_element_particle_new(piece->widget_interface->style_element, particle, 3,0);

	return piece;
}