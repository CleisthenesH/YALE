// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once

#include "widget_interface.h"

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

enum PIECE_UVALUE
{
	PIECE_UVALUE_ID = 1,
};

enum ZONE_UVALUE
{
	ZONE_UVALUE_ID = 1,
};

enum BOARD_MANAGER_UVALUE
{
	MANAGER_UVALUE_VALID_MOVES = 1,
	MANAGER_UVALUE_MOVE,
	MANAGER_UVALUE_PIECES,
	MANAGER_UVALUE_ZONES,
	MANAGER_UVALUE_NONVALID_MOVE,
};

struct piece
{
	struct widget_interface* widget_interface;
	const struct piece_jump_table* jump_table;

	void* upcast;
	struct zone* zone;
	struct board_manager* manager;
};

struct zone
{
	struct widget_interface* widget_interface;
	const struct zone_jump_table* jump_table;
	void* upcast;
	struct board_manager* manager;

	bool valid_move;
	bool highlighted;
	bool nominated;

	size_t allocated;
	size_t used;
	struct piece** pieces;
};

struct board_manager
{
	int self;
	struct piece* moving_piece;
	struct zone* originating_zone;

	bool auto_snap;					// When making a potential move automatically snap the piece valid zones.
	bool auto_highlight;			// When making a potential move automatically highlight valid zone.
	bool auto_transition;			// After making a move auto transition to the zones snap
	bool auto_block_invalid_moves;	// A zone can only be nominated if it is a vaild move. (Not implemented)
	bool auto_self_heightlight;		// Highlight the zone a piece comes fromz. (Not implemented)
};

struct zone_jump_table
{
	void (*gc)(struct zone* const);
	void (*draw)(const struct zone* const);
	void (*mask)(const struct zone* const);
	void (*update)(struct zone* const);

	void (*highligh_start)(struct zone* const);
	void (*highligh_end)(struct zone* const);

	void (*remove_piece)(struct zone* const, struct piece* const);
	void (*append_piece)(struct zone* const, struct piece* const);

	int (*index)(struct zone* const);
	int (*newindex)(struct zone* const);
};

struct piece_jump_table
{
	void (*gc)(struct piece* const);
	void (*draw)(const struct piece* const);
	void (*mask)(const struct piece* const);

	int (*index)(struct piece* const);
	int (*newindex)(struct piece* const);
};

struct piece* piece_new(void* upcast, const struct piece_jump_table* const jump_table);
struct zone* zone_new(void* upcast, const struct zone_jump_table* const jump_table);