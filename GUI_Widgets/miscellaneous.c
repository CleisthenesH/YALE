// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include <allegro5/allegro.h>
#include "allegro5/allegro_font.h"
#include <stdio.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

void al_draw_scaled_text(ALLEGRO_FONT* font,ALLEGRO_COLOR color,float x,float y,float dy,float scale,int flag,const char* text)
{
	const ALLEGRO_TRANSFORM* const buffer = al_get_current_transform();
	ALLEGRO_TRANSFORM tmp;

	al_build_transform(&tmp, 
		x, y - dy * scale, 
		scale, scale, 0);
	al_compose_transform(&tmp, buffer);

	al_use_transform(&tmp);

	al_draw_text(font,
		color,
		0, 0,
		ALLEGRO_ALIGN_CENTRE, text);

	al_use_transform(buffer);
}

void stack_dump(lua_State* L)
{
	int top = lua_gettop(L);
	printf("Stack Dump (%d):\n", top);
	for (int i = 1; i <= top; i++) {
		printf("\t%d\t%s\t", i, luaL_typename(L, i));
		switch (lua_type(L, i)) {
		case LUA_TNUMBER:
			printf("\t%g\n", lua_tonumber(L, i));
			break;
		case LUA_TSTRING:
			printf("\t%s\n", lua_tostring(L, i));
			break;
		case LUA_TBOOLEAN:
			printf("\t%s\n", (lua_toboolean(L, i) ? "true" : "false"));
			break;
		case LUA_TNIL:
			printf("\t%s\n", "nil");
			break;
		default:
			printf("\t%p\n", lua_topointer(L, i));
			break;
		}
	}
}

void invert_transform_3D(ALLEGRO_TRANSFORM* src)
{
#define DET(A, B, C, D, E, F, G, H) ((src->m[B][A])*(src->m[H][G])-(src->m[F][E])*(src->m[D][C]))
#define MULT(N) (-src->m[3][0]*buffer[0][N]-src->m[3][1]*buffer[1][N]-src->m[3][2]*buffer[2][N])

	float D = 1 / (src->m[0][0] * DET(1, 1, 1, 2, 2, 1, 2, 2)
		- src->m[1][0] * DET(1, 0, 1, 2, 2, 0, 2, 2)
		+ src->m[2][0] * DET(1, 0, 1, 1, 2, 0, 2, 1));

	float buffer[4][3];

	// Calculate rotation
	buffer[0][0] = DET(1, 1, 1, 2, 2, 1, 2, 2) * D;
	buffer[1][0] = DET(0, 2, 0, 1, 2, 2, 2, 1) * D;
	buffer[2][0] = DET(0, 1, 0, 2, 1, 1, 1, 2) * D;
	buffer[0][1] = DET(1, 2, 1, 0, 2, 2, 2, 0) * D;
	buffer[1][1] = DET(0, 0, 0, 2, 2, 0, 2, 2) * D;
	buffer[2][1] = DET(0, 2, 0, 0, 1, 2, 1, 0) * D;
	buffer[0][2] = DET(1, 0, 1, 1, 2, 0, 2, 1) * D;
	buffer[1][2] = DET(0, 1, 0, 0, 2, 1, 2, 0) * D;
	buffer[2][2] = DET(0, 0, 0, 1, 1, 0, 1, 1) * D;

	// Calculate translationd (must happen after rotation)
	buffer[3][0] = MULT(0);
	buffer[3][1] = MULT(1);
	buffer[3][2] = MULT(2);

	// Transfer from buffer to src
	src->m[0][0] = buffer[0][0];
	src->m[0][1] = buffer[0][1];
	src->m[0][2] = buffer[0][2];
	src->m[1][0] = buffer[1][0];
	src->m[1][1] = buffer[1][1];
	src->m[1][2] = buffer[1][2];
	src->m[2][0] = buffer[2][0];
	src->m[2][1] = buffer[2][1];
	src->m[2][2] = buffer[2][2];
	src->m[3][0] = buffer[3][0];
	src->m[3][1] = buffer[3][1];
	src->m[3][2] = buffer[3][2];

#undef DET
#undef MULT
}

void print_transform(ALLEGRO_TRANSFORM* src)
{
	printf("\n%f\t%f\t%f\t%f\n%f\t%f\t%f\t%f\n%f\t%f\t%f\t%f\n%f\t%f\t%f\t%f\n",
		src->m[0][0], src->m[1][0], src->m[2][0], src->m[3][0],
		src->m[0][1], src->m[1][1], src->m[2][1], src->m[3][1],
		src->m[0][2], src->m[1][2], src->m[2][2], src->m[3][2],
		src->m[0][3], src->m[1][3], src->m[2][3], src->m[3][3]);
}