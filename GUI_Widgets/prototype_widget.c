// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// A barebones widget for prototyping

#include "widget_interface.h"

#include "card_render_data.h"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include "allegro5/allegro_primitives.h"

struct prototype
{
	struct widget_interface* widget_interface;
	struct card_render_data* card;
};

static void draw(const struct widget_interface* const widget)
{
	struct prototype* const prototype = (struct prototype* const) widget->upcast;
	//al_draw_filled_circle(0, 0, 100, al_color_name("pink"));
	card_render_draw(prototype->card, widget->style_element);
}

static void mask(const struct widget_interface* const widget)
{
}

static void update(const struct widget_interface* const widget)
{
}

static const struct widget_jump_table prototype_jump_table_entry =
{
	.draw = draw,
	.mask = mask,
	.update = update,
};

int prototype_new(lua_State* L)
{
	struct prototype* const prototype = malloc(sizeof(struct prototype));

	if (!prototype)
		return 0;

	*prototype = (struct prototype)
	{
		.widget_interface = widget_interface_new(L,prototype,&prototype_jump_table_entry),
		.card = malloc(sizeof(struct card_render_data)),
	};

	*prototype->card = (struct card_render_data)
	{
		.name = "Alex",
		.level = 2,
	};

	return 1;
}
