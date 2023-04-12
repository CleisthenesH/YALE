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

// Zone

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

static void zone_gc(struct widget_interface* const widget)
{
	struct zone* const zone = (struct zone*)widget->upcast;

	if (zone->jump_table->gc)
		zone->jump_table->gc(zone);
}

static void zone_draw(const struct widget_interface* const widget)
{
	struct zone* const zone = (struct zone*)widget->upcast;
	zone->jump_table->draw(zone);
}

static void zone_mask(const struct widget_interface* const widget)
{
	struct zone* const zone = (struct zone*)widget->upcast;
	zone->jump_table->mask(zone);
}

static const struct widget_jump_table zone_to_widget_table =
{
	.uservalues = 1,

	.gc = zone_gc,
	.draw = zone_draw,
	.mask = zone_mask,

	/*
	.index = zone_index,
	.newindex = zone_new_index,

	.drag_end_drop = zone_drag_end_drop,
	.drop_start = zone_drop_start,
	.drop_end = zone_drop_end,
	*/
};

// Piece

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

static void piece_gc(struct widget_interface* const widget)
{
	struct piece* const piece = (struct piece*)widget->upcast;
	if (piece->jump_table->gc)
		piece->jump_table->gc(piece);
}

static void piece_draw(const struct widget_interface* const widget)
{
	const struct piece* const piece = (struct piece*)widget->upcast;
	piece->jump_table->draw(piece);
}

static void piece_mask(const struct widget_interface* const widget)
{
	const struct piece* const piece = (struct piece*)widget->upcast;
	piece->jump_table->mask(piece);
}

static const struct widget_jump_table piece_to_widget_table =
{
	.uservalues = 1,

	.gc = piece_gc,
	.draw = piece_draw,
	.mask = piece_mask,

	/*
	.index = pieces_index,
	.newindex = pieces_new_index,

	.hover_start = piece_hover_start,
	.hover_end = piece_hover_end,
	.drag_end_drop = piece_drag_end_drop,
	.drop_start = piece_drop_start,
	.drop_end = piece_drop_end,
	*/
};

// Board Manager

static inline struct zone* zone_factory(lua_State* const L, const char* type)
{
	if (strcmp(type, "square") == 0)
		return square_new(L);

	return NULL;
}

static inline int piece_manager_new_zone(lua_State* L)
{
	// Check arguments
	struct board_manager* const board_manager = (struct board_manager*)luaL_checkudata(L, -4, "board_manager_mt");

	if (!board_manager ||
		lua_type(L, -3) == LUA_TTABLE ||
		lua_type(L, -2) != LUA_TSTRING)
	{
		lua_settop(L, 0);

		return 0;
	}

	// Prepare for zone factory
	const char* type = luaL_checkstring(L, -2);
	lua_remove(L, -2);

	if (lua_type(L, -1) == LUA_TNIL)
		lua_pop(L, 1);

	struct zone* const zone = zone_factory(L, type);

	if (!zone)
	{
		lua_settop(L, 0);
		return 0;
	}

	zone->manager = board_manager;

	lua_pushvalue(L, -2);
	const int check = lua_setiuservalue(L, -2, ZONE_UVALUE_ID);

	lua_pushvalue(L, -1);
	lua_insert(L, -4);

	lua_getiuservalue(L, -3, MANAGER_UVALUE_ZONES);
	lua_replace(L, -4);

	lua_settable(L, -3);
	lua_pop(L, 1);

	return 1;
}

static inline struct piece* piece_factory(lua_State* const L, const char* type)
{
	if (strcmp(type, "checker") == 0)
		return checker_new(L);

	return NULL;
}

static inline int piece_manager_new_piece(lua_State* L)
{
	// Check arguments
	struct board_manager* const board_manager = (struct board_manager*)luaL_checkudata(L, -4, "board_manager_mt");
	
	if (!board_manager ||
		lua_type(L, -3) == LUA_TTABLE ||
		lua_type(L, -2) != LUA_TSTRING)
	{
		lua_settop(L, 0);

		return 0;
	}

	// Prepare for piece factory
	const char* type = luaL_checkstring(L, -2);
	lua_remove(L, -2);

	if (lua_type(L, -1) == LUA_TNIL)
		lua_pop(L, 1);
	
	struct piece* const piece = piece_factory(L, type);

	if (!piece)
	{
		lua_settop(L, 0);
		return 0;
	}

	piece->manager = board_manager;
	
	lua_pushvalue(L, -2);
	const int check = lua_setiuservalue(L, -2, PIECE_UVALUE_ID);

	lua_pushvalue(L, -1);
	lua_insert(L, -4);

	lua_getiuservalue(L, -3, MANAGER_UVALUE_PIECES);
	lua_replace(L, -4);

	lua_settable(L, -3);
	lua_pop(L, 1);

	return 1;
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

		if (strcmp(key, "test") == 0)
		{
			lua_rawgeti(main_lua_state, LUA_REGISTRYINDEX, board_manager->self);
			lua_getiuservalue(main_lua_state, -1, MANAGER_UVALUE_MOVE);
			lua_rotate(L, -2, 1);

			lua_pushnil(L);
			lua_call(main_lua_state, 2, 0);

			printf("test\n");
			return 0;
		}

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

		if (strcmp(key, "new_piece") == 0)
		{
			lua_pushcfunction(L, piece_manager_new_piece);
			return 1;
		}
		
		if (strcmp(key, "new_zone") == 0)
		{
			lua_pushcfunction(L, piece_manager_new_zone);
			return 1;
		}

		/*

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
// ( Currently runs duing lua init, before allegro init.)
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