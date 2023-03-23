// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// Need to update this code from previous 
//  - Type checking with "check_widget"
//  - Something broke when we changed to a weak widget table in lua
//  - After some expermentation I think that manager shouldn't be sent of the callbacks
//     - Headless mode might make it more intgratable

#include "pieces_manager.h"

extern lua_State* const main_lua_state;
extern double current_timestamp;

static const struct widget_jump_table piece_to_widget_table;
static const struct widget_jump_table zone_to_widget_table;

// Test widgets
extern struct piece* checker_new(lua_State*);
extern struct zone* square_new(lua_State*);

// Managing state functions

static inline void update_zones(bool input)
{
	lua_pushnil(main_lua_state);
	while (lua_next(main_lua_state, -2) != 0)
	{
		struct widget_interface* const widget = check_widget(main_lua_state, -1, &zone_to_widget_table);

		if (!widget)
		{
			lua_pop(main_lua_state, 1);
			continue;
		}

		struct zone* const zone = widget->upcast;

		zone->valid_move = input;

		if (zone->manager->auto_highlight)
		{
			zone->highlighted = input;

			if (input)
			{
				if (zone->jump_table->highligh_start)
					zone->jump_table->highligh_start(zone);
			}
			else
			{
				if (zone->jump_table->highligh_end)
					zone->jump_table->highligh_end(zone);

				zone->nominated = false;
			}
		}

		if(zone->manager->auto_snap)
			zone->widget_interface->is_snappable= input;

		lua_pop(main_lua_state, 1);
	}

}

static void call_valid_moves(lua_State* const L, struct piece* piece)
{
	// Convert the pointer to the widget 
	lua_getglobal(main_lua_state, "widgets");

	lua_rawgetp(main_lua_state, -1, piece->widget_interface);

	// Push the manager on the stack
	lua_rawgeti(main_lua_state, LUA_REGISTRYINDEX, piece->manager->self);

	// Push the zones onto the st
	lua_getiuservalue(main_lua_state, -1, MANAGER_UVALUE_ZONES);

	update_zones(false);

	lua_getiuservalue(main_lua_state, -2, MANAGER_UVALUE_VALID_MOVES);

	// Manupulate stack such that the top is:
	// function, state, piece
	lua_rotate(main_lua_state, -4, -2);
	lua_rotate(main_lua_state, -2, 1);

	// Call the pre_move function
	lua_call(main_lua_state, 2, 1);

	// Everything in the returned table 
	if (lua_istable(main_lua_state, -1))
		update_zones(true);

	// Clean up the table
	lua_pop(main_lua_state, 3);
}

static inline void zone_remove_piece(struct zone* const zone, struct piece* const piece)
{
	size_t i;

	for (i = 0; i < zone->used; i++)
		if (zone->pieces[i] == piece)
			break;

	for (size_t j = i + 1; j < zone->used; j++)
		zone->pieces[j - 1] = zone->pieces[j];

	zone->pieces[zone->used--] = NULL;
}

static inline void move_piece(struct zone* zone, struct piece* piece)
{
	// Convert the pointers to the udata
	lua_getglobal(main_lua_state, "widgets");
	lua_rawgetp(main_lua_state, -1, zone->widget_interface);
	lua_rawgetp(main_lua_state, -2, piece->widget_interface);

	// Push the manager on the stack
	lua_rawgeti(main_lua_state, LUA_REGISTRYINDEX, piece->manager->self);

	// Push the function on the stack
	lua_getiuservalue(main_lua_state, -1, MANAGER_UVALUE_MOVE);

	if (lua_iscfunction(main_lua_state, -1))
	{
		// Manipuate the stack such that the top is:
		// function, manager, piece, nil
		lua_rotate(main_lua_state, -4, 1);
		lua_rotate(main_lua_state, -3, 1);
		lua_rotate(main_lua_state, -2, 1);

		// Call the post_move function then clean the stack
		lua_call(main_lua_state, 3, 0);
		lua_pop(main_lua_state, 1);
	}
	else
	{
		lua_pop(main_lua_state, 5);
	}

	if (piece->zone)
	{
		if (piece->zone->jump_table->remove_piece)
			piece->zone->jump_table->remove_piece(piece->zone, piece);

		zone_remove_piece(piece->zone, piece);
	}

	if (zone->jump_table->append_piece)
		zone->jump_table->append_piece(zone, piece);

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

	if (zone->manager->auto_transition)
	{
		struct render_interface* const style = piece->widget_interface->style_element;
		struct keyframe keyframe;
		render_interface_interupt(style);

		render_interface_copy_destination(zone->widget_interface->style_element, &keyframe);
		keyframe.timestamp = current_timestamp + 0.2;
		render_interface_push_keyframe(style, &keyframe);
	}
}

static void default_state(lua_State* const L, struct manager* manager)
{
	// Push the manager on the stack
	lua_rawgeti(main_lua_state, LUA_REGISTRYINDEX, manager->self);
	lua_getiuservalue(main_lua_state, -1, MANAGER_UVALUE_ZONES);

	update_zones(false);
}

// Piece 

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

static void zone_gc(struct widget_interface* const widget)
{
	struct zone* const zone = (struct zone*)widget->upcast;

	if (zone->jump_table->gc)
		zone->jump_table->gc(zone);
}

static void piece_hover_start(struct widget_interface* const widget)
{
	call_valid_moves(main_lua_state, widget->upcast);
}

static void piece_hover_end(struct widget_interface* const widget)
{
	struct piece* const piece = (struct piece*)widget->upcast;

	default_state(main_lua_state,piece->manager);
}

static void piece_drag_start(struct widget_interface* const widget)
{
	// pop lol
}

static void piece_drag_end(struct widget_interface* const widget)
{
	struct piece* const piece = (struct piece*)widget->upcast;

	default_state(main_lua_state, piece->manager);
}

// TODO: clean up
// The zone shouldn't be dragged?
// How do I know that it's a zone I've been droped on?
// Need to add a buch of type checking.
void zone_drag_end_drop(struct widget_interface* const zone_widget, struct widget_interface* const piece_widget);
static void piece_drag_end_drop(struct widget_interface* const droppedon_widget, struct widget_interface* const piece_widget)
{
	struct piece* const dropped_on = droppedon_widget->upcast;

	if (dropped_on->zone)
		zone_drag_end_drop(dropped_on->zone->widget_interface, piece_widget);
}

static void piece_drop_start(struct widget_interface* const widget)
{
	struct zone* const zone = ((struct piece* const)widget->upcast)->zone;

	if (zone)
		zone->nominated = true;
}

static void piece_drop_end(struct widget_interface* const widget)
{
	struct zone* const zone = ((struct piece* const)widget->upcast)->zone;

	if (zone)
		zone->nominated = false;
}

static int pieces_index(lua_State* L)
{
	struct widget_interface* const widget = check_widget(L, -2, &piece_to_widget_table);
	struct piece* const piece = widget->upcast;

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -1);

		if (strcmp(key, "payload") == 0)
		{
			lua_getiuservalue(L, -2, PIECE_UVALUE_PAYLOAD);
			return 1;
		}

		if (strcmp(key, "zone") == 0)
		{
			if (!piece->zone)
				return 0;

			// Convert the pointer to the widget 
			lua_getglobal(main_lua_state, "widgets");
			lua_rawgetp(main_lua_state, -1, piece->zone->widget_interface);
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

static const struct widget_jump_table piece_to_widget_table =
{
	.uservalues = 1,

	.gc = piece_gc,
	.draw = piece_draw,
	.mask = piece_mask,

	.hover_start = piece_hover_start,
	.hover_end = piece_hover_end,
	.index = pieces_index,
	.newindex = pieces_new_index,
	.drag_end_drop = piece_drag_end_drop,
	.drop_start = piece_drop_start,
	.drop_end = piece_drop_end,
};

// Zone

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

static inline void zone_push_piece_to_stack(lua_State* const L, const struct zone* const zone)
{
	lua_getglobal(main_lua_state, "widgets");
	lua_createtable(L, (int) zone->used, 0);

	for (size_t i = 0; i < zone->used; i++)
	{
		lua_rawgetp(L, -2, zone->pieces[i]->widget_interface);
		lua_rawsetp(L, -2, zone->pieces[i]);
	}
}

static int zone_new_index(lua_State* L)
{
	struct widget_interface* const widget = check_widget(L, -3, &zone_to_widget_table);
	struct zone* const zone = widget->upcast;

	if (lua_type(L, -2) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -2);

		if (strcmp(key, "payload") == 0)
		{
			lua_setiuservalue(L, -3, ZONE_UVALUE_PAYLOAD);
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
			zone_push_piece_to_stack(L, zone);
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

static void zone_drag_end_drop(struct widget_interface* const zone_widget, struct widget_interface* const piece_widget)
{
	// Check that the drop is a valid move
	struct zone* const zone = zone_widget->upcast;
	struct piece* const piece = piece_widget->upcast;

	// If it's not a vaild move don't
	if (!zone->valid_move)
		return;

	move_piece(zone, piece);
	call_valid_moves(main_lua_state, piece);
	zone->nominated = false;
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

static const struct widget_jump_table zone_to_widget_table =
{
	.uservalues = 1,

	.gc = zone_gc,
	.draw = zone_draw,
	.mask = zone_mask,
	//.drag_end_drop = zone_drag_end_drop,
	.drop_start = zone_drop_start,
	.drop_end = zone_drop_end,
	.index = zone_index,
	.newindex = zone_new_index,
};

// Piece Manager

static int piece_manager_move(lua_State* L)
{
	struct manager* const manager = (struct manager*)luaL_checkudata(L, -3, "piece_manager_mt");
	struct widget_interface* zone_widget =  check_widget(L, -1, &zone_to_widget_table);
	struct widget_interface* piece_widget = check_widget(L, -2, &piece_to_widget_table);

	move_piece(zone_widget->upcast, piece_widget->upcast);

	return 0;
}

static inline int piece_manager_new_zone(lua_State* L)
{
	struct manager* const manager = (struct manager*)luaL_checkudata(L, -2, "piece_manager_mt");
	stack_dump(L);

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -1);

		if (strcmp(key, "square") == 0)
		{
			lua_getiuservalue(L, -2, MANAGER_UVALUE_ZONES);
			struct zone* const zone = square_new(L);


			/* asigning the pointer to some table, I think this is outdated code
			lua_pushvalue(L, -1);
			stack_dump(L);
			lua_rawsetp(L, -3, zone);
			*/

			zone->manager = manager;
			
			return 1;
		}
	}

	return 0;
}

static inline int piece_manager_new_piece(lua_State* L)
{
	struct manager* const manager = (struct manager*)luaL_checkudata(L, -2, "piece_manager_mt");

	if (lua_type(L, -1) == LUA_TSTRING)
	{
		const char* key = luaL_checkstring(L, -1);

		if (strcmp(key, "checker") == 0)
		{
			lua_getiuservalue(L, -2, MANAGER_UVALUE_PIECES);
			struct piece* const piece = checker_new(L);
			piece->manager = manager;

			/* asigning the pointer to some table, I think this is outdated code
			lua_pushvalue(L, -1);
			lua_rawsetp(L, -3, piece);
			*/

			return 1;
		}
	}

	return 0;
}

static int piece_manager_newindex(lua_State* L)
{
	struct manager* const manager = (struct manager*)luaL_checkudata(L, -3, "piece_manager_mt");

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
			manager->auto_snap = lua_toboolean(L, -1);
			return 0;
		}

		if (strcmp(key, "auto_highlight") == 0)
		{
			manager->auto_highlight = lua_toboolean(L, -1);
			return 0;
		}
	}

	return 0;
}

static int piece_manager_index(lua_State* L)
{
	struct manager* const piece_manager = (struct manager*)luaL_checkudata(L, -2, "piece_manager_mt");

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

		if (strcmp(key, "move") == 0)
		{
			lua_pushcfunction(L, piece_manager_move);
			return 1;
		}
	}

	return 0;
}

static int piece_manager_new(lua_State* L)
{
	struct manager* const piece_manager = lua_newuserdatauv(L, sizeof(struct manager), 5);

	if (!piece_manager)
		return 0;

	*piece_manager = (struct manager)
	{
		.auto_highlight = true,
		.auto_snap = true,
		.auto_transition = true,
	};

	lua_pushvalue(L, -1);
	piece_manager->self = luaL_ref(L, LUA_REGISTRYINDEX);

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