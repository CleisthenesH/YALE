// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

extern double current_timestamp;
extern double delta_timestamp;

// Return the current time and delta
static int get_current_time(lua_State* L)
{
    lua_pushnumber(L, current_timestamp);
    lua_pushnumber(L, delta_timestamp);

    return 2;
}

// Set misc lua interface globals
void lua_openL_misc(lua_State* L)
{
    lua_pushcfunction(L, get_current_time);
    lua_setglobal(L, "current_time");
}
