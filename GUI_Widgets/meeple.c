// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "board_manager.h"
#include "resource_manager.h"

struct meeple
{
	ALLEGRO_COLOR color;
};

static void gc(struct piece* const meeple)
{

}

static void draw(const struct piece* const piece)
{
	const double half_width = piece->widget_interface->render_interface->half_width;
	const double half_height = piece->widget_interface->render_interface->half_height;
	struct meeple* const meeple = (struct meeple* const) piece->upcast;

	al_draw_tinted_scaled_bitmap(resource_manager_icon(ICON_ID_ABBOT_MEEPLE), 
		meeple->color,
		0, 0, 512, 512, 
		-half_width, -half_height, 2 * half_width, 2 * half_height, 
		0);
}

static void mask(const struct piece* const meeple)
{
	const double half_width = meeple->widget_interface->render_interface->half_width;
	const double half_height = meeple->widget_interface->render_interface->half_height;

	al_draw_scaled_bitmap(resource_manager_icon(ICON_ID_ABBOT_MEEPLE),
		0, 0, 512, 512,
		-half_width, -half_height, 2 * half_width, 2 * half_height, 
		0);
}

static struct piece_jump_table meeple_table =
{
	.draw = draw,
	.mask = mask,
	.gc = gc,
};

struct piece* meeple_new(lua_State* L)
{
	struct meeple* meeple = malloc(sizeof(struct meeple));

	if (!meeple)
		return NULL;

	*meeple = (struct meeple)
	{
		.color = al_map_rgb(0x00,0x00,0xFF),
	};

	struct piece* piece = piece_new(L, meeple, &meeple_table);

	piece->widget_interface->render_interface->half_width = 50;
	piece->widget_interface->render_interface->half_height = 50;

	return piece;
}