// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

#define FOR_WIDGETS(DO) \
	DO(rectangle) \
	//DO(card) \

extern lua_State* main_lua_state;
extern double current_timestamp;
extern double delta_timestamp;

#define EXTERN(widget) extern int widget ## _new(lua_State*); extern int widget_ ## _make_metatable(lua_State*);

FOR_WIDGETS(EXTERN)

int get_current_time(lua_State* L)
{
	lua_pushnumber(L, current_timestamp);
	lua_pushnumber(L, delta_timestamp);

	return 2;
}

#define LUA_REG_ENTRY(widget) {#widget , widget ## _new},

static const struct luaL_Reg lib_f[] = {
	FOR_WIDGETS(LUA_REG_ENTRY)
	{"current_time",get_current_time},
	{NULL,NULL}
};

void widget_lua_integration()
{
#define CALL_METATABLE(widget) widget ## _make_metatable(main_lua_state); 
	FOR_WIDGETS(CALL_METATABLE)

	luaL_newlib(main_lua_state, lib_f);
	lua_setglobal(main_lua_state, "widget_engine");
}