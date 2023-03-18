// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once

#include "style_element.h"

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

struct widget_jump_table
{
	size_t uservalues;

	void (*gc)(struct widget_interface* const);

	void (*draw)(const struct widget_interface* const);
	void (*update)(struct widget_interface* const);
	void (*event_handler)(struct widget_interface* const);
	void (*mask)(const struct widget_interface* const);

	void (*hover_start)(struct widget_interface* const);
	void (*hover_end)(struct widget_interface* const);

	void (*left_click)(struct widget_interface* const);
	void (*left_click_end)(struct widget_interface* const);
	void (*right_click)(struct widget_interface* const);
	void (*click_off)(struct widget_interface* const);

	void (*drag_start)(struct widget_interface* const);
	void (*drag_end_no_drop)(struct widget_interface* const);
	void (*drag_end_drop)(struct widget_interface* const, struct widget_interface* const);
	void (*drop_start)(struct widget_interface* const, struct widget_interface* const);
	void (*drop_end)(struct widget_interface* const, struct widget_interface* const);

	int (*index)(lua_State* L);
	int (*newindex)(lua_State* L);
};

struct widget_interface
{
	struct render_interface* style_element;
	void* upcast;

	bool is_draggable;
	bool is_snappable;
};

// TODO: an reorder functions + widget containter class

struct widget_interface* widget_interface_new(
	lua_State* L,
	const void* const upcast,
	const struct widget_jump_table* const jump_table);

struct widget_interface* check_widget(lua_State*, int , const struct widget_jump_table* const );
	
void widget_screen_to_local(const struct widget_interface* const, double*, double*);

void stack_dump(lua_State*);

