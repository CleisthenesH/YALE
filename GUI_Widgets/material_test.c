// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// A barebones widget for prototyping

#include "widget_interface.h"

#include "card_render_data.h"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include "allegro5/allegro_primitives.h"

extern double mouse_x, mouse_y;

struct material_test
{
	struct widget_interface* widget_interface;
	struct material* material;

	int effect_id, selection_id;
};

static void draw(const struct widget_interface* const widget)
{
	struct material_test* const handle = (struct material_test* const)widget->upcast;

	style_element_apply_material(widget->style_element, handle->material);

	al_draw_filled_rounded_rectangle(-90, -127, 90, 127, 10, 10, al_color_name("forestgreen"));
}

static void mask(const struct widget_interface* const widget)
{
	al_draw_filled_rounded_rectangle(-90, -127, 90, 127, 10, 10, al_color_name("forestgreen"));
}

static void update(const struct widget_interface* const widget)
{
	struct material_test* const handle = (struct material_test* const)widget->upcast;
	material_effect_point(handle->material, mouse_x, mouse_y);
}

static const struct widget_jump_table material_test_table_entry =
{
	.draw = draw,
	.mask = mask,
	.update = update,
};

int material_test_new(lua_State* L)
{
	struct material_test* const handle = malloc(sizeof(struct material_test));
	int material_id = 0, selection_id = 0;

	lua_getfield(L, -1, "effect");

	if (lua_isinteger(L, -1))
		material_id = luaL_checkinteger(L, -1);

	lua_getfield(L, -2, "selection");

	if (lua_isinteger(L, -1))
		selection_id = luaL_checkinteger(L, -1);

	if (material_id >= EFFECT_ID_MAX)
		material_id = 0;

	if (selection_id >= EFFECT_ID_MAX)
		selection_id = 0;
	
	lua_pop(L, 3);

	if (!handle)
		return 0;

	*handle = (struct material_test)
	{
		.widget_interface = widget_interface_new(L,handle,&material_test_table_entry),
		.material = material_new(material_id, selection_id),
		.effect_id = material_id,
		.selection_id = selection_id
	};

	handle->widget_interface->is_draggable = true;

	return 1;
}
