// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "board_manager.h"
#include "allegro5/allegro_primitives.h"

struct square
{
	ALLEGRO_COLOR color;
};

static void gc(struct zone* const square)
{

}

static void draw(const struct zone* const square)
{
	if(square->nominated)
		al_draw_filled_rectangle(-50, -50, 50, 50, al_map_rgb_f(1, 1, 0));
	else 
		if (square->highlighted)
			al_draw_filled_rectangle(-50, -50, 50, 50, al_map_rgb(255, 105, 180));
		else
			al_draw_filled_rectangle(-50, -50, 50, 50, ((struct square*)square->upcast)->color);
}

static void mask(const struct zone* const square)
{
	al_draw_filled_rectangle(-50, -50, 50, 50, al_map_rgb_f(1, 1, 1));
}

static struct zone_jump_table square_table =
{
	.draw = draw,
	.mask = mask,
	.gc = gc,
};

struct zone* square_new(lua_State* L)
{
	 struct square* const square = malloc(sizeof(struct square));

	if (!square)
		return 0;

	if (lua_istable(L, -1))
	{
		lua_getfield(L, -1, "color");

		if (!lua_isnil(L, -1))
		{
			lua_geti(L, -1, 1);
			lua_geti(L, -2, 2);
			lua_geti(L, -3, 3);

			const int r = luaL_checkinteger(L, -3);
			const int g = luaL_checkinteger(L, -2);
			const int b = luaL_checkinteger(L, -1);

			square->color = al_map_rgb(r, g, b);

			lua_pop(L, 3);
		}
		else
			square->color = al_map_rgb(255, 255, 255);

		lua_pop(L, 1);
	}

	struct zone* zone = zone_new(square, &square_table);

	zone->widget_interface->render_interface->half_width = 50;
	zone->widget_interface->render_interface->half_height = 50;

	return zone;
}