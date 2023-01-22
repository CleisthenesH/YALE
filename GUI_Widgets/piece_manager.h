// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once

#include "widget_interface.h"

#include <stdint.h>
#include <stdbool.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

struct game_zone_jump_table
{
	void (*gc)(struct game_zone* const);
	void (*draw)(const struct game_zone* const);
	void (*mask)(const struct game_zone* const);
};

struct game_zone
{
	struct widget_interface* widget_interface;
	const struct game_zone_jump_table* jump_table;
	void* upcast;

	int payload;
	int game_pieces;
	int piece_manager;
};

struct game_piece_jump_table
{
	void (*gc)(struct game_piece* const);
	void (*draw)(const struct game_piece* const);
	void (*mask)(const struct game_piece* const);
	// maybe add a function that pushes custom relevent to dragable context to the stack
};

struct game_piece
{
	struct widget_interface* widget_interface;
	const struct game_piece_jump_table* jump_table;

	void* upcast;

	int payload;
	int game_zones; // A table of game_zones the piece is in
	int piece_manager;
};

struct piece_manager
{
	int self;

	int piece_drag_start;
	int zone_drag_start;
	int zone_drag_end;
	int zone_drag_drop;
};

struct game_zone* zone_new(lua_State* L, void* upcast,
	const struct game_zone_jump_table* const jump_table);

struct game_piece* piece_new(lua_State* L, void* upcast,
	const struct game_piece_jump_table* const jump_table);

// Some inits
