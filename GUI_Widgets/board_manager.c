// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#include "board_manager.h"

extern lua_State* const main_lua_state;
extern double current_timestamp;

static const struct widget_jump_table piece_to_widget_table;
static const struct widget_jump_table zone_to_widget_table;

// Test widgets
extern struct piece* checker_new(lua_State*);
extern struct zone* square_new(lua_State*); 

struct piece* piece_new(lua_State* L, void* upcast, const struct piece_jump_table* const jump_table)
{
	struct piece* const piece = malloc(sizeof(struct piece));

	if (!piece)
		return 0;

	*piece = (struct piece)
	{
		.widget_interface = widget_interface_new(L,piece,&piece_to_widget_table),
		.jump_table = jump_table,
		.upcast = upcast
	};

	piece->widget_interface->is_draggable = true;

	return piece;
}

struct zone* zone_new(lua_State* L, void* upcast, const struct zone_jump_table* const jump_table)
{
	struct zone* const zone = malloc(sizeof(struct zone));

	if (!zone)
		return 0;

	*zone = (struct zone)
	{
		.widget_interface = widget_interface_new(L,zone,&zone_to_widget_table),
		.jump_table = jump_table,
		.upcast = upcast
	};

	return zone;
}

static int board_manager_newindex(lua_State* L)
{
	struct board_manager* const board_manager = (struct board_manager*)luaL_checkudata(L, -3, "board_manager_mt");

	if (lua_type(L, -2) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -2);

		if (strcmp(key, "vaild_moves") == 0)
		{
			lua_setiuservalue(L, -3, MANAGER_UVALUE_VALID_MOVES);
			return 1;
		}

		if (strcmp(key, "move") == 0)
		{
			lua_setiuservalue(L, -3, MANAGER_UVALUE_MOVE);
			return 1;
		}

		if (strcmp(key, "invalid_move") == 0)
		{
			lua_setiuservalue(L, -3, MANAGER_UVALUE_INVALID_MOVE);
			return 1;
		}

		if (strcmp(key, "auto_snap") == 0)
		{
			board_manager->auto_snap = lua_toboolean(L, -1);
			return 0;
		}

		if (strcmp(key, "auto_highlight") == 0)
		{
			board_manager->auto_highlight = lua_toboolean(L, -1);
			return 0;
		}
	}

	return 0;
}

static int board_manager_index(lua_State* L)
{
	struct board_manager* const board_manager = (struct board_manager*) luaL_checkudata(L, -2, "board_manager_mt");

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -1);

		if (strcmp(key, "pieces") == 0)
		{
			lua_getiuservalue(L, -2, MANAGER_UVALUE_PIECES);
			return 1;
		}

		if (strcmp(key, "zones") == 0)
		{
			lua_getiuservalue(L, -2, MANAGER_UVALUE_ZONES);
			return 1;
		}

		/*
		if (strcmp(key, "new_zone") == 0)
		{
			lua_pushcfunction(L, piece_manager_new_zone);
			return 1;
		}

		if (strcmp(key, "new_piece") == 0)
		{
			lua_pushcfunction(L, piece_manager_new_piece);
			return 1;
		}

		if (strcmp(key, "move") == 0)
		{
			lua_pushcfunction(L, piece_manager_move);
			return 1;
		}
		*/
	}

	return 0;
}

// Construct a new board manager.
// Read a table as config, if one is present.
static int board_manager_new(lua_State* L)
{
	struct board_manager* const board_manager = lua_newuserdatauv(L, sizeof(struct board_manager), 5);

	if (!board_manager)
		return 0;

	// Asign board manager defaults 
	*board_manager = (struct board_manager)
	{
		.auto_highlight = true,
		.auto_snap = true,
		.auto_transition = true,
		.auto_block_invalid_moves = true,
	};

	// If there table read config from it
	if (lua_type(L, 1) == LUA_TTABLE)
	{
		lua_rotate(L, -2, 1);

		if (lua_getfield(L, 2, "snap") != LUA_TNIL)
			board_manager->auto_snap = lua_toboolean(L, -1);

		if (lua_getfield(L, 2, "highlight") != LUA_TNIL)
			board_manager->auto_highlight = lua_toboolean(L, -1);

		if (lua_getfield(L, 2, "transition") != LUA_TNIL)
			board_manager->auto_transition = lua_toboolean(L, -1);

		if (lua_getfield(L, 2, "block_invalid_move") != LUA_TNIL)
			board_manager->auto_block_invalid_moves = lua_toboolean(L, -1);
		lua_pop(L, 5);
	}

	// Put a value of the lua object into the registary and take a reference
	lua_pushvalue(L, -1);
	board_manager->self = luaL_ref(L, LUA_REGISTRYINDEX);

	// Set metatable
	luaL_getmetatable(L, "board_manager_mt");
	lua_setmetatable(L, -2);

	// Set uvalues for pieces and zones
	lua_newtable(L);
	lua_setiuservalue(L, -2, MANAGER_UVALUE_PIECES);

	lua_newtable(L);
	lua_setiuservalue(L, -2, MANAGER_UVALUE_ZONES);

	return 1;
}

// Initalize the given lua state to construct and manipulate board managers.
// ( Currently runs duing lua init, ie before allegro init.)
void board_manager_init(lua_State* L)
{
	// global contructor
	lua_pushcfunction(L, board_manager_new);
	lua_setglobal(L, "board_manager_new");

	// Make the board manager meta table
	luaL_newmetatable(L, "board_manager_mt");

	const struct luaL_Reg meta_table[] = {
		{"__index",board_manager_index},
		{"__newindex",board_manager_newindex},
		{NULL,NULL}
	};

	luaL_setfuncs(L, meta_table, 0);
}