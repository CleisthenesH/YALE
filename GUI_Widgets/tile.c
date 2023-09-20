// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "board_manager.h"
#include "resource_manager.h"

struct tile
{
	ALLEGRO_COLOR color;
};

static void gc(struct zone* const zone)
{

}

static void draw(const struct zone* const zone)
{
	const double half_width = zone->widget_interface->render_interface->half_width;
	const double half_height = zone->widget_interface->render_interface->half_height;
	struct tile* const tile = (struct tile* const) zone->upcast;

	al_draw_tinted_scaled_bitmap(resource_manager_tile(TILE_HILLS),
		tile->color,
		0, 0, 300, 300, 
		-half_width, -half_height, 2*half_width, 2*half_height, 
		0);
}

static void mask(const struct zone* const zone)
{
	const double half_width = zone->widget_interface->render_interface->half_width;
	const double half_height = zone->widget_interface->render_interface->half_height;

	al_draw_scaled_bitmap(resource_manager_tile(TILE_HILLS), 
		0, 0, 300, 300, 
		-half_width, -half_height, 2*half_width, 2*half_height, 
		0);
}

static struct zone_jump_table tile_table =
{
	.draw = draw,
	.mask = mask,
	.gc = gc,
};

struct zone* tile_new(lua_State* L)
{
	struct tile* tile = malloc(sizeof(struct tile));

	if (!tile)
		return NULL;

	*tile = (struct tile)
	{
		.color = al_map_rgb(0x00,0xAA,0xFF),
	};

	struct zone* zone = zone_new(tile, &tile_table);

	zone->widget_interface->render_interface->half_width = 50;
	zone->widget_interface->render_interface->half_height = 50;

	return zone;
}