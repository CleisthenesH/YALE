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
	void (*gc)(struct game_piece* const);
	void (*draw)(const struct game_piece* const);
	void (*mask)(const struct game_piece* const);
};

struct game_zone
{
	struct widget_interface* widget_interface;
	const struct game_zone_jump_table* jump_table;
	void* upcast;

	uint8_t owner;
	uint8_t controler;
	uint8_t type;		// Like inplay, inhand
	uint8_t subtype;	// Like a counter

	bool droppable_zone;
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

	struct game_zone* location;
	struct piece_manager* state;

	uint8_t type;
	uint8_t subtype;
	uint8_t owner;
	uint8_t controler;

	int is_dragable_funct_ref;
	int valild_zones_funct_ref;
};

struct piece_manager
{
	int piece_table_ref;
	int zone_table_ref;
};

void zone_new(lua_State* L, void* upcast,
	const struct game_zone_jump_table* const jump_table);

void piece_new(lua_State* L, void* upcast,
	const struct game_piece_jump_table* const jump_table);

// Some inits
