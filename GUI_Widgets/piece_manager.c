// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "piece_manager.h"

extern lua_State* const main_lua_state;

static const struct widget_jump_table piece_to_widget_table;
static const struct widget_jump_table zone_to_widget_table;

extern void checker_new(lua_State*);
extern void square_new(lua_State*);

enum PIECE_UVALUE
{
	PIECE_UVALUE_PAYLOAD = 1,
	PIECE_UVALUE_ZONE,
	PIECE_UVALUE_MANAGER,
};

enum ZONE_UVALUE
{
	ZONE_UVALUE_PAYLOAD = 1,
	ZONE_UVALUE_PIECES,
	ZONE_UVALUE_MANAGER,
};

enum MANAGER_UVALUE
{
	MANAGER_UVALUE_PIECES = 1,
	MANAGER_UVALUE_ZONES,
	MANAGER_UVALUE_PREMOVE,
	MANAGER_UVALUE_POSTMOVE
};

struct zone
{
	struct widget_interface* widget_interface;
	const struct zone_jump_table* jump_table;
	void* upcast;
};

struct piece
{
	struct widget_interface* widget_interface;
	const struct piece_jump_table* jump_table;

	void* upcast;
};

static void piece_gc(struct widget_interface* const widget)
{
	 struct piece* const piece = (struct piece*) widget->upcast;
	 if(piece->jump_table->gc)
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

static void piece_hover_start(struct widget_interface* const widget)
{
}

static void piece_hover_end(struct widget_interface* const widget)
{
	// some type of default snap and dragable ness?
}

static void piece_drag_start(struct widget_interface* const widget)
{
	// Convert the pointer to the widget 
	lua_getglobal(main_lua_state, "widgets");
	lua_pushlightuserdata(main_lua_state, widget);
	lua_gettable(main_lua_state, -2);

	// Push the manager on the stack
	lua_getiuservalue(main_lua_state, -1, PIECE_UVALUE_MANAGER);

	// Push the zones onto the st
	lua_getfield(main_lua_state, -1, "zones");

	// All zones are not snappable by default
	lua_pushnil(main_lua_state);  
	while (lua_next(main_lua_state, -2) != 0) 
	{
		lua_pushboolean(main_lua_state, false);
		lua_setfield(main_lua_state,-2,"snappable");

		lua_pop(main_lua_state, 1);
	}

	// Push the function on the stack
	lua_getiuservalue(main_lua_state, -2, MANAGER_UVALUE_PREMOVE);

	// Manupulate stack such that the top is:
	// function, state, piece
	lua_rotate(main_lua_state, -4, -2);
	lua_rotate(main_lua_state, -2, 1);

	// Call the pre_move function
	lua_call(main_lua_state, 2, 1);

	// Everything in the returned table 
	if (lua_istable(main_lua_state, -1))
	{
		lua_pushnil(main_lua_state);
		while (lua_next(main_lua_state, -2) != 0)
		{
			lua_pushboolean(main_lua_state, true);
			lua_setfield(main_lua_state, -2, "snappable");

			lua_pop(main_lua_state, 1);
		}
	}

	// Clean up the table
	lua_pop(main_lua_state, 3);
}

static void piece_drag_end_no_drop(struct widget_interface* const widget)
{
	// Not actually a move, shouldn't trigger an event
	if (0)
	{
		// Convert the pointer to the widget 
		lua_getglobal(main_lua_state, "widgets");
		lua_pushlightuserdata(main_lua_state, widget);
		lua_gettable(main_lua_state, -2);

		// Push the manager on the stack
		lua_getiuservalue(main_lua_state, -1, 3);

		// Push the function on the stack
		lua_getiuservalue(main_lua_state, -1, 4);

		// Manipuate the stack such that the top is:
		// function, manager, piece, nil
		lua_rotate(main_lua_state, -3, 1);
		lua_rotate(main_lua_state, -2, 1);
		lua_pushnil(main_lua_state);

		// Call the post_move function
		lua_call(main_lua_state, 3, 0);

		// clean the stack
		lua_pop(main_lua_state, 1);
	}
}

static int pieces_index(lua_State* L)
{
	struct widget_interface* const widget = check_widget(L, -2, &piece_to_widget_table);
	struct piece* const piece = widget->upcast;

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -1);

		if (strcmp(key, "zone") == 0)
		{
			lua_getiuservalue(L, -2, PIECE_UVALUE_ZONE);
			return 1;
		}

		if (strcmp(key, "manager") == 0)
		{
			lua_getiuservalue(L, -2, PIECE_UVALUE_MANAGER);
			return 1;
		}

		if (strcmp(key, "payload") == 0)
		{
			lua_getiuservalue(L, -2, PIECE_UVALUE_PAYLOAD);
			return 1;
		}
	}

	return 0;
}

static int pieces_new_index(lua_State* L)
{
	struct widget_interface* const widget = check_widget(L, -3, &piece_to_widget_table);
	struct piece* const piece = widget->upcast;

	if (lua_type(L, -2) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -2);

		if (strcmp(key, "payload") == 0)
		{
			lua_setiuservalue(L, -3, PIECE_UVALUE_PAYLOAD);
			return 0;
		}
	}
	return 0;

}

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

	lua_pushlightuserdata(L, piece);
	lua_rotate(L, -2, 1);

	// pushes a lightudata and udata to the stack (in that order, light under normal)
	return piece;
}

static void zone_gc(struct widget_interface* const widget)
{
	struct zone* const zone = (struct zone*)widget->upcast;

	if(zone->jump_table->gc)
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

static void zone_drag_end_drop(struct widget_interface* const zone, struct widget_interface* const piece)
{
	// Check that the drop is a valid move
	if (!zone->is_snappable)
		return;

	// Convert the pointers to the widgets
	lua_getglobal(main_lua_state, "widgets");
	lua_pushlightuserdata(main_lua_state, zone);
	lua_gettable(main_lua_state, -2);
	lua_pushlightuserdata(main_lua_state, piece);
	lua_gettable(main_lua_state, -3);

	// Push the manager on the stack
	lua_getiuservalue(main_lua_state, -1, PIECE_UVALUE_MANAGER);

	// Push the function on the stack
	lua_getiuservalue(main_lua_state, -1, MANAGER_UVALUE_POSTMOVE);

	// Manipuate the stack such that the top is:
	// function, manager, piece, nil
	lua_rotate(main_lua_state, -4, 1);
	lua_rotate(main_lua_state, -3, 1);
	lua_rotate(main_lua_state, -2, 1);

	// Call the post_move function
	lua_call(main_lua_state, 3, 0);

	// The stack is now just the global widget 
	
	// Push the piece udata onto the stack
	lua_pushlightuserdata(main_lua_state, piece);
	lua_gettable(main_lua_state, -2);

	// Check if a table is assigned to piece
	lua_getiuservalue(main_lua_state, -1, PIECE_UVALUE_ZONE);

	// If the piece was previously in a zone remove it
	if (!lua_isnil(main_lua_state, -1))
	{
		lua_getiuservalue(main_lua_state, -1, ZONE_UVALUE_PIECES);

		lua_pushlightuserdata(main_lua_state, piece);
		lua_pushnil(main_lua_state);
		lua_settable(main_lua_state, -3);

		lua_pop(main_lua_state, 4);
	}
	else
		lua_pop(main_lua_state, 2);

	// Push the zone udata back onto the stack
	lua_pushlightuserdata(main_lua_state, zone);
	lua_gettable(main_lua_state, -2);

	// Push the zone's piece table onto the stack
	lua_getiuservalue(main_lua_state, -1, ZONE_UVALUE_PIECES);

	// Push the pieces lightudata and udata onto the stack
	lua_pushlightuserdata(main_lua_state, piece);
	lua_pushvalue(main_lua_state, -1);
	lua_gettable(main_lua_state, -5);

	// Assign the pieces to the zones table
	lua_settable(main_lua_state, -3);
	
	lua_pop(main_lua_state, 3);
}

static int zone_index(lua_State* L)
{
	struct widget_interface* const widget = check_widget(L, -2, &zone_to_widget_table);
	struct zone* const zone = widget->upcast;

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -1);

		if (strcmp(key, "pieces") == 0)
		{
			lua_getiuservalue(L, -2, ZONE_UVALUE_PIECES);
			return 1;
		}

		if (strcmp(key, "manager") == 0)
		{
			lua_getiuservalue(L, -2, ZONE_UVALUE_MANAGER);
			return 1;
		}

		if (strcmp(key, "payload") == 0)
		{
			lua_getiuservalue(L, -2, ZONE_UVALUE_PAYLOAD);
			return 1;
		}
	}

	return 0;
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

	lua_pushlightuserdata(L, zone);
	lua_rotate(L, -2, 1);

	// set the piece table to an empty table
	lua_newtable(L);
	lua_setiuservalue(L, -2, ZONE_UVALUE_PIECES);

	return zone;
}

static const struct widget_jump_table zone_to_widget_table =
{
	.gc = zone_gc,
	.draw = zone_draw,
	.mask = zone_mask,
	.drag_end_drop = zone_drag_end_drop,
	.index = zone_index,
	.uservalues = 3,
};

static const struct widget_jump_table piece_to_widget_table =
{
	.gc = piece_gc,
	.draw = piece_draw,
	.mask = piece_mask,
	.hover_start = piece_hover_start,
	.hover_end = piece_hover_end,
	.index = pieces_index,
	.newindex = pieces_new_index,
	.drag_start = piece_drag_start,
	.drag_end_no_drop = piece_drag_end_no_drop,
	.uservalues = 3,
};

static inline int piece_manager_new_piece(lua_State* L)
{
	struct piece_manager* const piece_manager = (struct piece_manager*)luaL_checkudata(L, -2, "piece_manager_mt");

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -1);

		if (strcmp(key, "checker") == 0)
		{
			lua_getiuservalue(L, -2, MANAGER_UVALUE_PIECES);
			checker_new(L);
			lua_pushvalue(L, -5);
			lua_setiuservalue(L, -2, PIECE_UVALUE_MANAGER);

			lua_pushvalue(L, -1);
			lua_rotate(L, -3, 1);
			lua_settable(L, -4);

			return 1;
		}
	}

	return 0;
}

static inline int piece_manager_new_zone(lua_State* L)
{
	struct piece_manager* const piece_manager = (struct piece_manager*)luaL_checkudata(L, -2, "piece_manager_mt");

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -1);

		if (strcmp(key, "square") == 0)
		{
			lua_getiuservalue(L, -2, MANAGER_UVALUE_ZONES);
			square_new(L);
			lua_pushvalue(L, -5);
			lua_setiuservalue(L, -2, ZONE_UVALUE_MANAGER);

			lua_pushvalue(L, -1);
			lua_rotate(L, -3, 1);
			lua_settable(L, -4);

			return 1;
		}
	}

	return 0;
}

static int piece_manager_newindex(lua_State* L)
{
	struct piece_manager* const piece_manager = (struct piece_manager*)luaL_checkudata(L, -3, "piece_manager_mt");

	if (lua_type(L, -2) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -2);
		
		if (strcmp(key, "pre_move") == 0)
		{
			lua_setiuservalue(L, -3, MANAGER_UVALUE_PREMOVE);
			return 1;
		}

		if (strcmp(key, "post_move") == 0)
		{
			lua_setiuservalue(L, -3, MANAGER_UVALUE_POSTMOVE);
			return 1;
		}
	}

	return 0;
}

static int piece_manager_index(lua_State* L)
{
	struct piece_manager* const piece_manager = (struct piece_manager*)luaL_checkudata(L, -2, "piece_manager_mt");

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
	}

	return 0;
}

static int piece_manager_new(lua_State* L)
{
	// TODO: implements hints for zone and pice size
	struct piece_manager* const piece_manager = lua_newuserdatauv(L,0,4);

	if (!piece_manager)
		return 0;

	// TODO: implment an enum for uservalue n
	luaL_getmetatable(L, "piece_manager_mt");
	lua_setmetatable(L, -2);

	lua_newtable(L);
	lua_setiuservalue(L, -2, MANAGER_UVALUE_PIECES);

	lua_newtable(L);
	lua_setiuservalue(L, -2, MANAGER_UVALUE_ZONES);

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
		{"__index",piece_manager_index},
		{"__newindex",piece_manager_newindex},
		{NULL,NULL}
	};

	luaL_setfuncs(L, meta_table, 0);
}
