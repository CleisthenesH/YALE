// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

extern lua_State* main_lua_state;

extern int rectangle_new(lua_State*);
extern int rectangle_make_metatable(lua_State*); 

static const struct luaL_Reg lib_f[] = {
	{"rectangle",rectangle_new},
	{NULL,NULL}
};

void widget_lua_integration()
{
	rectangle_make_metatable(main_lua_state);

	luaL_newlib(main_lua_state, lib_f);
	lua_setglobal(main_lua_state, "widget_engine");
}