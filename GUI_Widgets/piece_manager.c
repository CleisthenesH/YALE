// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "piece_manager.h"

extern lua_State* const main_lua_state;

// game_piece integration functions

static void piece_gc(struct widget_interface* const widget)
{
	 struct game_piece* const game_piece = (struct game_piece*) widget->upcast;
	 if(game_piece->jump_table->gc)
		game_piece->jump_table->gc(game_piece);
}

static void piece_draw(const struct widget_interface* const widget)
{
	const struct game_piece* const game_piece = (struct game_piece*)widget->upcast;
	game_piece->jump_table->draw(game_piece);
}

static void piece_mask(const struct widget_interface* const widget)
{
	const struct game_piece* const game_piece = (struct game_piece*)widget->upcast;
	game_piece->jump_table->mask(game_piece);
}

static void piece_hover_start(struct widget_interface* const widget)
{
	struct game_piece* const game_piece = (struct game_piece*)widget->upcast;
/*
	// Check if the game_piece is draggable
	if (game_piece->is_dragable_funct_ref != LUA_REFNIL)
	{
		lua_rawgeti(main_lua_state, LUA_REGISTRYINDEX, game_piece->is_dragable_funct_ref); 

		lua_pushinteger(main_lua_state, game_piece->type);
		lua_pushinteger(main_lua_state, game_piece->subtype);
		lua_pushinteger(main_lua_state, game_piece->controler);
		lua_pushinteger(main_lua_state, game_piece->owner);

		if (lua_pcall(main_lua_state, 4, 1, 0) == LUA_OK && !lua_isnil(main_lua_state, -1))
			widget->is_draggable = (luaL_checkinteger(main_lua_state,-1) != 0);
		// TODO: handle errors
	}
	*/

	// Update snappable zones
}

static void piece_hover_end(const struct widget_interface* const widget)
{
	// some type of default snap and dragable ness?
}

static const struct widget_jump_table game_piece_to_widget_table =
{
	.gc = piece_gc,
	.draw = piece_draw,
	.mask = piece_mask,
	.hover_start = piece_hover_start,
	.hover_end = piece_hover_end,
};

// game_piece general functions

struct game_piece* piece_new(lua_State* L, void* upcast,
	const struct game_piece_jump_table* const jump_table)
{
	struct game_piece* const piece = malloc(sizeof(struct game_piece));

	if (!piece)
		return 0;

	*piece = (struct game_piece)
	{
		.widget_interface = widget_interface_new(L,piece,&game_piece_to_widget_table),
		.jump_table = jump_table,
		.upcast = upcast
	};

	lua_pushlightuserdata(L, piece);
	lua_rotate(L, -2, 1);

	// pushes a lightudata and udata to the stack (in that order, light under normal)
	return piece;
}

extern void checker_new(lua_State*);

static int piece_new_lua(lua_State* L)
{
	struct piece_manager* const piece_manager = (struct piece_manager*)luaL_checkudata(L, -2, "piece_manager_mt");
	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -1);

		if (strcmp(key, "checker") == 0)
		{
			lua_getiuservalue(L, -2, 1);
			checker_new(L);
			lua_settable(L, -3);
			
			return 1;
		}
	}

	return 0;
}

// game_zone functions

static void zone_gc(struct widget_interface* const widget)
{
	struct game_zone* const game_zone = (struct game_zone*)widget->upcast;

	if(game_zone->jump_table->gc)
		game_zone->jump_table->gc(game_zone);
}

static void zone_draw(const struct widget_interface* const widget)
{
	struct game_zone* const game_zone = (struct game_zone*)widget->upcast;
	game_zone->jump_table->draw(game_zone);
}

static void zone_mask(const struct widget_interface* const widget)
{
	struct game_zone* const game_zone = (struct game_zone*)widget->upcast;
	game_zone->jump_table->mask(game_zone);
}

static void zone_drag_end_drop(struct widget_interface* const zone, struct widget_interface* const piece)
{

}

static const struct widget_jump_table game_zone_to_widget_table =
{
	.gc = zone_gc,
	.draw = zone_draw,
	.mask = zone_mask,
	.drag_end_drop = zone_drag_end_drop,
};

struct game_zone* zone_new(lua_State* L, void* upcast,
	const struct game_zone_jump_table* const jump_table)
{
	struct game_zone* const zone = malloc(sizeof(struct game_zone));

	if (!zone)
		return 0;

	*zone = (struct game_zone)
	{
		.widget_interface = widget_interface_new(L,zone,&game_zone_to_widget_table),
		.jump_table = jump_table,
		.upcast = upcast
	};

	lua_pushlightuserdata(L, zone);
	lua_rotate(L, -2, 1);

	return zone;
}

extern void square_new(lua_State*);

static int zone_new_lua(lua_State* L)
{
	struct piece_manager* const piece_manager = (struct piece_manager*)luaL_checkudata(L, -2, "piece_manager_mt");
	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -1);

		if (strcmp(key, "square") == 0)
		{
			lua_getiuservalue(L, -2, 2);
			square_new(L);
			lua_settable(L, -3);

			return 1;
		}
	}

	return 0;
}

// Geneal widget index method
static int index(lua_State* L)
{
	struct piece_manager* const piece_manager = (struct piece_manager*)luaL_checkudata(L, -2, "piece_manager_mt");

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -1);

		if (strcmp(key, "pieces") == 0)
		{
			lua_getiuservalue(L, -2, 1);
			return 1;
		}

		if (strcmp(key, "zones") == 0)
		{
			lua_getiuservalue(L, -2, 2);
			return 1;
		}

		if (strcmp(key, "new_zone") == 0)
		{
			lua_pushcfunction(L, zone_new_lua);
			return 1;
		}

		if (strcmp(key, "new_piece") == 0)
		{
			lua_pushcfunction(L, piece_new_lua);
			return 1;
		}

		if (strcmp(key, "test") == 0)
		{
			lua_getuservalue(L, 1);	
			return 1;
		}
	}

	return 0;
}

static int piece_manager_new(lua_State* L)
{
	// TODO: implements hints
	struct piece_manager* const piece_manager = lua_newuserdatauv(L, sizeof(struct piece_manager),2);

	if (!piece_manager)
		return 0;

	luaL_getmetatable(L, "piece_manager_mt");
	lua_setmetatable(L, -2);

	lua_newtable(L);
	lua_setiuservalue(L, -2, 1);

	lua_newtable(L);
	lua_setiuservalue(L, -2, 2);

	return 1;
}

void piece_manager_setglobal(lua_State* L)
{
	// global contructor
	lua_pushcfunction(L, piece_manager_new);
	lua_setglobal(L, "piece_manager_new");

	// Make the pieces manager meta table
	luaL_newmetatable(L, "piece_manager_mt");

	const struct luaL_Reg meta_table[] = {
		{"__index",index},
		{NULL,NULL}
	};

	luaL_setfuncs(L, meta_table, 0);
}
