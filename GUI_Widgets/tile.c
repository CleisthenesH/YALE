// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "board_manager.h"
#include "resource_manager.h"
#include "meeple_tile_utility.h"

struct tile
{
	enum TEAMS team;
	enum tile_id id;
};

static void gc(struct zone* const zone)
{

}

static inline enum TILE_PALLET state_to_pallet(struct zone* zone)
{
	if (zone->nominated)
		return TILE_PALLET_NOMINATED;

	if (zone->highlighted)
		return TILE_PALLET_HIGHLIGHTED;

	return TILE_PALLET_IDLE;
}

static void draw(const struct zone* const zone)
{
	const double half_width = zone->widget_interface->render_interface->half_width;
	const double half_height = zone->widget_interface->render_interface->half_height;
	struct tile* const tile = (struct tile* const) zone->upcast;

	al_draw_scaled_bitmap(resource_manager_tile(TILE_EMPTY),
		0, 0, 300, 300,
		-half_width, -half_height, 2 * half_width, 2 * half_height,
		0);

	if (tile->id == TILE_EMPTY)
		return;

	ALLEGRO_COLOR tile_pallet[TEAM_CNT][TILE_PALLET_CNT] =
	{
		{al_color_name("White"),al_color_name("Khaki"),al_color_name("Gold")},
		{al_color_name("tomato"),al_color_name("crimson"),al_color_name("brown")},
		{al_color_name("lightblue"),al_color_name("royalblue"),al_color_name("steelblue")},
	};

	al_draw_tinted_scaled_bitmap(resource_manager_tile(tile->id),
		tile_pallet[tile->team][state_to_pallet(zone)],
		0, 0, 300, 300, 
		-half_width, -half_height, 2*half_width, 2*half_height, 
		0);
}

static void mask(const struct zone* const zone)
{
	const double half_width = zone->widget_interface->render_interface->half_width;
	const double half_height = zone->widget_interface->render_interface->half_height;

	al_draw_scaled_bitmap(resource_manager_tile(TILE_EMPTY), 
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

// 
static void lua_toteam(lua_State* L, int idx, struct tile* tile)
{
	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* team_name = luaL_tolstring(L, idx, NULL);

		if (strcmp(team_name, "red") == 0)
			tile->team = TEAM_RED;
		else if (strcmp(team_name, "blue") == 0)
			tile->team = TEAM_BLUE;

		lua_pop(L, 1);
	}

}

// 
static void lua_toid(lua_State* L, int idx, struct tile* tile)
{
	const char* lookup[] = {
		"empty",
		"bridge",
		"camp",
		"castle",
		"city",
		"dungeon",
		"farm",
		"fort",
		"hills",
		"lake",
		"mine",
		"monolith",
		"mountains",
		"oak",
		"oaks",
		"pine",
		"pines",
		"poi",
		"quest",
		"ruins",
		"shipwreck",
		"skull",
		"swamp",
		"tower",
		"town"
	};

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* tile_id = luaL_tolstring(L, idx, NULL);

		for (size_t i = 0; i<TILE_CNT; i++)
			if (strcmp(tile_id, lookup[i]) == 0)
			{
				tile->id = i;
				break;
			}

		lua_pop(L, 1);
	}

}

struct zone* tile_new(lua_State* L)
{
	struct tile* tile = malloc(sizeof(struct tile));

	if (!tile)
		return NULL;

	*tile = (struct tile)
	{
		.team = TEAM_NONE,
		.id = TILE_EMPTY,
	};

	if (lua_istable(L, -1))
	{
		lua_getfield(L, -1, "team");
		lua_toteam(L, -1, tile);

		stack_dump(L);

		lua_getfield(L, -2, "tile");
		lua_toid(L, -1, tile);

		lua_pop(L, 2);
	}

	struct zone* zone = zone_new(tile, &tile_table);

	zone->widget_interface->render_interface->half_width = 50;
	zone->widget_interface->render_interface->half_height = 50;

	return zone;
}