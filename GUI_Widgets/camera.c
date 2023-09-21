// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "camera.h"
#include "tweener.h"

static struct tweener* camera_tweener;

void camera_global_predraw()
{
	al_build_transform(&camera_transform,
		camera_tweener->current[0],
		camera_tweener->current[1],
		camera_tweener->current[2],
		camera_tweener->current[3],
		camera_tweener->current[4]
	);
}

void camera_push_keyframe(const struct keyframe* const frame)
{
	double* new_point = tweener_new_point(camera_tweener);

	new_point[0] = frame->timestamp;
	new_point[1] = frame->x;
	new_point[2] = frame->y;
	new_point[3] = frame->sx;
	new_point[4] = frame->sy;
	new_point[5] = frame->theta;
}

void camera_set_keyframe(const struct keyframe* const set)
{
	tweener_set(camera_tweener, (double[]) { set->x, set->y, set->sx, set->sy, set->theta });
}

void camera_interupt()
{
	tweener_interupt(camera_tweener);
}

void camera_compose_transform(ALLEGRO_TRANSFORM* const trans, const double blend)
{
	ALLEGRO_TRANSFORM buffer;

	const double blend_x = camera_tweener->current[0] * blend;
	const double blend_y = camera_tweener->current[1] * blend;
	const double blend_sx = camera_tweener->current[2] * blend +(1-blend);
	const double blend_sy = camera_tweener->current[3] * blend+(1-blend);
	const double blend_theta = camera_tweener->current[4] * blend;

	al_build_transform(&buffer,
		blend_x, blend_y,
		blend_sx, blend_sy,
		blend_theta);

	al_compose_transform(trans, &buffer);
}

void camera_copy_destination(struct keyframe* const keyframe)
{
	double* coords =  tweener_destination(camera_tweener);

	keyframe->timestamp = coords[0];
	keyframe->x = coords[1];
	keyframe->y = coords[2];
	keyframe->sx = coords[3];
	keyframe->sy = coords[4];
	keyframe->theta = coords[5];
}

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

extern lua_State* main_lua_state;

static int set_keyframe(lua_State* L)
{
	struct widget_interface* const widget = (struct widget_interface*)luaL_checkudata(L, -2, "widget_mt");

	struct keyframe keyframe;

	keyframe_default(&keyframe);
	lua_tokeyframe(L, &keyframe);

	camera_set_keyframe(&keyframe);

	return 0;
}

static int current_keyframe(lua_State* L)
{
	struct keyframe keyframe = {
		.x = camera_tweener->current[0],
		.y = camera_tweener->current[1],
		.sx = camera_tweener->current[2],
		.sy = camera_tweener->current[3],
		.theta = camera_tweener->current[4],
	};

	lua_pushkeyframe(L, &keyframe);

	return 1;
}

static int push_keyframe(lua_State* L)
{
	luaL_checktype(L, -1, LUA_TTABLE);

	struct keyframe keyframe;
	keyframe_default(&keyframe);

	lua_tokeyframe(L, &keyframe);
	camera_push_keyframe(&keyframe);

	return 1;
}

static int destination_keyframe(lua_State* L)
{
	struct keyframe keyframe;
	camera_copy_destination(&keyframe);
	lua_pushkeyframe(L, &keyframe);

	return 0;
}

void camera_init()
{
	camera_tweener = tweener_new(5, 0);

	tweener_set(camera_tweener, (double[]) { 0, 0, 1, 1, 0 });

	lua_newtable(main_lua_state);
	lua_newtable(main_lua_state);

	lua_pushvalue(main_lua_state, -1);
	lua_setfield(main_lua_state, -2, "__index");

	const struct luaL_Reg camera_methods[] = {
		{"set",set_keyframe},
		{"current",current_keyframe},
		{"push",push_keyframe},
		{"destination",destination_keyframe},
		{NULL,NULL}
	};

	luaL_setfuncs(main_lua_state, camera_methods, 0);

	lua_setmetatable(main_lua_state, -2);
	lua_setglobal(main_lua_state, "camera");
}