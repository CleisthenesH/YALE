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

// State Management

// Return the zones to the default state
static void default_zones(lua_State* const L, int manager_indx)
{
	lua_getiuservalue(L, manager_indx, MANAGER_UVALUE_ZONES);

	lua_pushnil(L);

	while(lua_next(L, -2) != 0)
	{
		struct widget_interface* const widget = check_widget(L, -1, &zone_to_widget_table);

		if (!widget)
		{
			lua_pop(L, 1);
			continue;
		}

		struct zone* const zone = widget->upcast;

		zone->valid_move = false;
		zone->nominated = false;

		if (zone->manager->auto_snap)
			zone->widget_interface->is_snappable = false;

		if (zone->highlighted)
		{
			if (zone->jump_table->highligh_end)
				zone->jump_table->highligh_end(zone);

			zone->highlighted = false;
		}

		lua_pop(L, 1);
	}

	lua_pop(L, 1);
}

// Call the vaild moves function
static void call_valid_moves(lua_State* const L, struct piece* piece)
{
	lua_getglobal(L, "widgets");

	lua_rawgeti(L, LUA_REGISTRYINDEX, piece->manager->self);

	lua_getiuservalue(L, -1, MANAGER_UVALUE_VALID_MOVES);

	if (lua_isnil(L, -1))
	{
		lua_pop(L, 3);

		return;
	}

	default_zones(L, -2);
	
	// Prepare the stack for function call:
	// ..., function, manager, piece ID
	lua_pushvalue(L, -2);

	lua_rawgetp(L, -4, piece->widget_interface);
	lua_getiuservalue(L, -1, PIECE_UVALUE_ID);

	lua_remove(L, -2);

	// Call the valid move function
	lua_call(L, 2, 1);

	// A table of vaild zones should be returned
	if (lua_istable(L, -1))
	{
		lua_getiuservalue(L, -2, MANAGER_UVALUE_ZONES);

		lua_pushnil(L);

		while (lua_next(L, -3) != 0)
		{
			lua_gettable(L, -3);

			struct widget_interface* const widget = check_widget(L, -1, &zone_to_widget_table);

			if (!widget)
			{
				lua_pop(L, 1);
				continue;
			}

			struct zone* const zone = widget->upcast;

			zone->valid_move = true;

			if (zone->manager->auto_highlight)
			{
				zone->highlighted = true;

				if (zone->jump_table->highligh_start)
					zone->jump_table->highligh_start(zone);
			}

			if (zone->manager->auto_snap)
				zone->widget_interface->is_snappable = true;

			lua_pop(L, 1);
		}

		lua_pop(L, 1);
	}

	lua_pop(L, 3);
}

// Removes a piece from the zone, not from the board
static inline void remove_piece(struct zone* const zone, struct piece* const piece)
{
	size_t i;

	//TODO: this can be optimized
	for (i = 0; i < zone->used; i++)
		if (zone->pieces[i] == piece)
			break;

	for (size_t j = i + 1; j < zone->used; j++)
		zone->pieces[j - 1] = zone->pieces[j];

	zone->pieces[zone->used--] = NULL;

	piece->zone = NULL;
}

// Append a piece to a zone
static inline void append_piece(struct zone* const zone, struct piece* const piece)
{
	if (zone->allocated <= zone->used)
	{
		const size_t new_cnt = 1.2 * zone->allocated + 1;

		struct piece** memsafe_hande = realloc(zone->pieces, new_cnt * sizeof(struct piece*));

		if (!memsafe_hande)
			return;

		zone->pieces = memsafe_hande;
		zone->allocated = new_cnt;
	}

	zone->pieces[zone->used++] = piece;
	piece->zone = zone;
}

// Call the appropriate (valid/nonvalid) move callback for the given zone and piece
static inline void moves_callback(lua_State* const L, struct zone* zone, struct piece* piece, bool vaild)
{
	// Push the manager on the stack
	lua_rawgeti(L, LUA_REGISTRYINDEX, piece->manager->self);

	// Push the function on the stack
	if(vaild)
		lua_getiuservalue(L, -1, MANAGER_UVALUE_MOVE);
	else
		lua_getiuservalue(L, -1, MANAGER_UVALUE_NONVALID_MOVE);

	// Organize argument and call the move function
	if (lua_isfunction(L, -1))
	{
		// Manipuate the stack such that the top is:
		// function, manager, piece id, zone id
		lua_rotate(L, -2, 1);

		lua_getglobal(L, "widgets");

		// Get piece ID
		lua_rawgetp(L, -1, piece->widget_interface);
		lua_getiuservalue(L, -1, PIECE_UVALUE_ID);
		lua_remove(L, -2);

		// Get zone ID
		lua_rawgetp(L, -2, zone->widget_interface);
		lua_getiuservalue(L, -1, ZONE_UVALUE_ID);
		lua_remove(L, -2);
		lua_remove(L, -3);

		// Call the post_move function, ballances the stack
		lua_call(L, 3, 0);
	}
	else
	{
		lua_pop(L, 2);
	}
}

// Update the zone and pieces structs in responce to a move
static inline void move_piece(struct zone* zone, struct piece* piece)
{
	// If the piece was in a zone remove it from the old zone
	if (piece->zone)
	{
		if (piece->zone->jump_table->remove_piece)
			piece->zone->jump_table->remove_piece(piece->zone, piece);

		remove_piece(piece->zone, piece);
	}

	// If there is a new zone append the pice to it.
	if (zone)
	{
		if (zone->jump_table->append_piece)
			zone->jump_table->append_piece(zone, piece);

		append_piece(zone, piece);

		// If the auto_transition flag is set transition the piece to be over the zone.
		if (zone->manager->auto_transition)
		{
			struct render_interface* const style = piece->widget_interface->render_interface;
			struct keyframe keyframe;
			render_interface_interupt(style);

			render_interface_copy_destination(zone->widget_interface->render_interface, &keyframe);
			keyframe.timestamp = current_timestamp + 0.2;
			render_interface_push_keyframe(style, &keyframe);
		}
	}
}

// Returns a manager to the default state
static void default_state(lua_State* const L, struct board_manager* manager)
{
	// Push the manager on the stack
	lua_rawgeti(L, LUA_REGISTRYINDEX, manager->self);
	lua_getiuservalue(L, -1, MANAGER_UVALUE_ZONES);

	default_zones(L,-2);

	lua_pop(L, 2);
}

// Process a manual move from lua
static int manual_move(lua_State* L)
{
	// Process the ids on the stack into widgets
	lua_getiuservalue(L, -3, MANAGER_UVALUE_PIECES);
	lua_getiuservalue(L, -4, MANAGER_UVALUE_ZONES);
	lua_remove(L, -5);
	lua_rotate(L, -4, 2);
	lua_gettable(L, -3);
	lua_insert(L, -2);
	lua_gettable(L, -4);

	// Grab the widgets from the stack
	struct widget_interface* zone = check_widget(L, -2, &zone_to_widget_table);
	struct widget_interface* piece = check_widget(L, -1, &piece_to_widget_table);

	// Ballance stack
	lua_pop(L, 4);

	// If the piece exists move it
	if (piece)
		move_piece(zone->upcast, piece->upcast);

	return 0;
}

// Pushes a table to the stack where the keys are the pieces in a vone and the value is their widget
static inline void push_piece_table(lua_State* const L, const struct zone* const zone)
{
	lua_getglobal(main_lua_state, "widgets");
	lua_createtable(L, (int)zone->used, 0);

	for (size_t i = 0; i < zone->used; i++)
	{
		lua_rawgetp(L, -2, zone->pieces[i]->widget_interface);
		lua_getiuservalue(L, -1, PIECE_UVALUE_ID);
		lua_settable(L, -3);
	}
}

// Zone
//TODO: somechecking that the widget being dropped/hovered is a zone/piece

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

static void zone_drop_start(struct widget_interface* const zone_widget, struct widget_interface* const piece_widget)
{
	struct zone* const zone = zone_widget->upcast;

	zone->nominated = true;
}

static void zone_drop_end(struct widget_interface* const zone_widget, struct widget_interface* const piece_widget)
{
	struct zone* const zone = zone_widget->upcast;

	zone->nominated = false;
}

static void zone_drag_end_drop(struct widget_interface* const zone_widget, struct widget_interface* const piece_widget)
{

	// Check that the drop is a valid move
	struct zone* const zone = zone_widget->upcast;
	struct piece* const piece = piece_widget->upcast;

	// If it's not a vaild move, don't
	if (!zone->valid_move)
	{
		moves_callback(main_lua_state, zone, piece, false);
		return;
	}

	move_piece(zone, piece);
	moves_callback(main_lua_state, zone, piece, true);
	call_valid_moves(main_lua_state, piece);
	zone->nominated = false;
}

static int zone_new_index(lua_State* L)
{
	struct widget_interface* const widget = check_widget(L, -3, &zone_to_widget_table);
	struct zone* const zone = widget->upcast;

	if (lua_type(L, -2) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -2);

		if (strcmp(key, "id") == 0)
		{
			lua_setiuservalue(L, -3, ZONE_UVALUE_ID);
			return 0;
		}
	}
	return 0;
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
			push_piece_table(L, zone);
			return 1;
		}

		if (strcmp(key, "id") == 0)
		{
			lua_getiuservalue(L, -2, ZONE_UVALUE_ID);
			return 1;
		}
	}

	return 0;
}

static const struct widget_jump_table zone_to_widget_table =
{
	.uservalues = 1,

	.gc = zone_gc,
	.draw = zone_draw,
	.mask = zone_mask,

	.drop_start = zone_drop_start,
	.drop_end = zone_drop_end,
	.drag_end_drop = zone_drag_end_drop,

	.index = zone_index,
	.newindex = zone_new_index,
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

static void piece_hover_start(struct widget_interface* const widget)
{
	call_valid_moves(main_lua_state, widget->upcast);
}

static void piece_hover_end(struct widget_interface* const widget)
{
	struct piece* const piece = (struct piece*)widget->upcast;

	default_state(main_lua_state, piece->manager);
}

static void piece_drop_start(struct widget_interface* const widget, struct widget_interface* const _)
{
	struct zone* const zone = ((struct piece* const)widget->upcast)->zone;

	if (zone)
		zone->nominated = true;
}

static void piece_drop_end(struct widget_interface* const widget, struct widget_interface* const _)
{
	struct zone* const zone = ((struct piece* const)widget->upcast)->zone;

	if (zone)
		zone->nominated = false;
}

static void piece_drag_end_drop(struct widget_interface* const droppedon_widget, struct widget_interface* const piece_widget)
{
	struct piece* const dropped_on = droppedon_widget->upcast;

	if (dropped_on->zone)
		zone_drag_end_drop(dropped_on->zone->widget_interface, piece_widget);
}

static int piece_index(lua_State* L)
{
	struct widget_interface* const widget = check_widget(L, -2, &piece_to_widget_table);
	struct piece* const piece = widget->upcast;

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -1);

		if (strcmp(key, "id") == 0)
		{
			lua_getiuservalue(L, -2, PIECE_UVALUE_ID);
			return 1;
		}

		if (strcmp(key, "zone") == 0)
		{
			if (!piece->zone)
				return 0;

			// Convert the pointer to the widget 
			lua_getglobal(L, "widgets");
			lua_rawgetp(L, -1, piece->zone->widget_interface);
			lua_getiuservalue(L, -1, ZONE_UVALUE_ID);

			lua_rotate(L, -5, 1);
			lua_pop(L, 4);
			return 1;
		}
	}

	return 0;
}

static int piece_new_index(lua_State* L)
{
	struct widget_interface* const widget = check_widget(L, -3, &piece_to_widget_table);
	struct piece* const piece = widget->upcast;

	if (lua_type(L, -2) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -2);

		if (strcmp(key, "id") == 0)
		{
			lua_setiuservalue(L, -3, PIECE_UVALUE_ID);
			return 0;
		}
	}

	return 0;
}

static const struct widget_jump_table piece_to_widget_table =
{
	.uservalues = 1,

	.gc = piece_gc,
	.draw = piece_draw,
	.mask = piece_mask,
	
	.hover_start = piece_hover_start,
	.hover_end = piece_hover_end,

	.drop_start = piece_drop_start,
	.drop_end = piece_drop_end,
	.drag_end_drop = piece_drag_end_drop,

	.index = piece_index,
	.newindex = piece_new_index,
};

// Board Manager

static inline struct zone* zone_factory(lua_State* const L, const char* type)
{
	if (strcmp(type, "square") == 0)
		return square_new(L);

	return NULL;
}

static inline int board_manager_new_zone(lua_State* L)
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

static inline int board_manager_new_piece(lua_State* L)
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

		if (strcmp(key, "nonvalid_move") == 0)
		{
			lua_setiuservalue(L, -3, MANAGER_UVALUE_NONVALID_MOVE);
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

		if (strcmp(key, "new_piece") == 0)
		{
			lua_pushcfunction(L, board_manager_new_piece);
			return 1;
		}
		
		if (strcmp(key, "new_zone") == 0)
		{
			lua_pushcfunction(L, board_manager_new_zone);
			return 1;
		}

		if (strcmp(key, "move") == 0)
		{
			lua_pushcfunction(L, manual_move);
			return 1;
		}
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