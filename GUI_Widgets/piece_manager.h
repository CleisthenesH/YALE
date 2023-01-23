// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once

#include "widget_interface.h"

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

struct zone_jump_table
{
	void (*gc)(struct zone* const);
	void (*draw)(const struct zone* const);
	void (*mask)(const struct zone* const);
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
