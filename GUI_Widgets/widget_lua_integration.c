// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

extern lua_State* main_lua_state;
extern double current_timestamp;
extern double delta_timestamp;

extern int rectangle_new(lua_State*);
extern int rectangle_make_metatable(lua_State*); 

int get_current_time(lua_State* L)
{
	lua_pushnumber(L, current_timestamp);
	lua_pushnumber(L, delta_timestamp);

	return 2;
}

static const struct luaL_Reg lib_f[] = {
	{"rectangle",rectangle_new},
	{"current_time",get_current_time},
	{NULL,NULL}
};

void widget_lua_integration()
{
	rectangle_make_metatable(main_lua_state);

	luaL_newlib(main_lua_state, lib_f);
	lua_setglobal(main_lua_state, "widget_engine");
}