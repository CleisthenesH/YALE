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
	MANAGER_UVALUE_POSTMOVE,
	MANAGER_UVALUE_FLOATING_PIECE,
};

struct zone
{
	struct widget_interface* widget_interface;
	const struct zone_jump_table* jump_table;
	void* upcast;

	bool highlight;
};

struct piece
{
	struct widget_interface* widget_interface;
	const struct piece_jump_table* jump_table;

	void* upcast;
};

struct zone_jump_table
{
	void (*gc)(struct zone* const);
	void (*draw)(const struct zone* const);
	void (*mask)(const struct zone* const);
	void (*update)(struct zone* const);

	void (*highligh_start)(struct zone* const);
	void (*highligh_end)(struct zone* const);
};

struct piece_jump_table
{
	void (*gc)(struct piece* const);
	void (*draw)(const struct piece* const);
	void (*mask)(const struct piece* const);
};

struct zone* zone_new(lua_State* L, void* upcast,
	const struct zone_jump_table* const jump_table);

struct piece* piece_new(lua_State* L, void* upcast,
	const struct piece_jump_table* const jump_table);
