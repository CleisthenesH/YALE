// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once

#include "style_element.h"

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

struct widget_interface
{
	struct style_element* style_element;
	void* upcast;

	void (*hover_start)(struct widget_interface* const);
	void (*hover_end)(struct widget_interface* const);
	void (*left_click)(struct widget_interface* const);
	void (*right_click)(struct widget_interface* const);
	void (*click_off)(struct widget_interface* const);
	void (*drag_start)(struct widget_interface* const);
	void (*drag_end_drop)(struct widget_interface* const, struct widget_interface* const);
	void (*drag_end_no_drop)(struct widget_interface* const);
	void (*drop_start)(struct widget_interface* const);
	void (*drop_end)(struct widget_interface* const);

	bool is_draggable;
	bool is_snappable;
};
  
struct widget_interface* widget_interface_new(
	lua_State*,
	const void* const,
	void (*)(const struct widget_interface* const),
	void (*)(struct widget_interface* const),
	void (*)(struct widget_interface* const),
	void (*)(const struct widget_interface* const));

