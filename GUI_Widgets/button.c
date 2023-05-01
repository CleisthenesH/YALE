// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include "resource_manager.h"
#include "widget_style_sheet.h"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include "allegro5/allegro_primitives.h"

extern ALLEGRO_FONT* test_font;

struct button
{
	struct widget_interface* widget_interface;
	char* text;
	ALLEGRO_FONT* font;
	ALLEGRO_COLOR color;
};

static void draw(const struct widget_interface* const widget)
{
	const struct button* const button = (struct button*) widget->upcast;
	const double half_width = 0.5 * button->widget_interface->style_element->width;
	const double half_height = 0.5 * button->widget_interface->style_element->height;

	al_draw_filled_rounded_rectangle(-half_width, -half_height, half_width, half_height,
		primary_pallet.edge_radius, primary_pallet.edge_radius,
		button->color);

	if (button->text)
		al_draw_text(button->font, al_map_rgb_f(1, 1, 1),
			0, -0.5 * al_get_font_line_height(button->font),
			ALLEGRO_ALIGN_CENTRE, button->text);

	al_draw_rounded_rectangle(-half_width, -half_height, half_width, half_height,
		primary_pallet.edge_radius, primary_pallet.edge_radius,
		primary_pallet.edge, primary_pallet.edge_width);
}

static void mask(const struct widget_interface* const widget)
{
	const struct button* const button = (struct button*)widget->upcast;
	const double half_width = 0.5 * button->widget_interface->style_element->width;
	const double half_height = 0.5 * button->widget_interface->style_element->height;

	al_draw_filled_rounded_rectangle(-half_width, -half_height, half_width, half_height,
		primary_pallet.edge_radius, primary_pallet.edge_radius,
		al_map_rgb(255, 255, 255));
}

static void hover_start(struct widget_interface* const widget)
{
	struct button* const button = (struct button*)widget->upcast;

	button->color = primary_pallet.highlight;
}

static void hover_end(struct widget_interface* const widget)
{
	struct button* const button = (struct button*)widget->upcast;

	button->color = primary_pallet.main;
}

static const struct widget_jump_table button_jump_table_entry =
{
	.draw = draw,

	// TODO: Investigate why seting mask to draw causes such a strange error. 
	// Shouldn't it all be set to a uniform color by the shader?
	.mask = mask,
	.hover_start = hover_start,
	.hover_end = hover_end,
};

int button_new(lua_State* L)
{
	struct button* const button = malloc(sizeof(struct button));

	if (!button)
		return 0;

	char* text = NULL;

	if (lua_istable(L, -1))
	{
		lua_getfield(L, -1, "text");

		if (lua_isstring(L, -1))
		{
			size_t text_len;
			char* buffer = lua_tolstring(L, -1, &text_len);

			text = malloc(sizeof(char) * (text_len + 1));
			strcpy_s(text, text_len + 1, buffer);
		}
		else
		{
			text = "Placeholder";
		}

		lua_pop(L, 1);
	}

	*button = (struct button)
	{
		.widget_interface = widget_interface_new(L,button,&button_jump_table_entry),
		.font = resource_manager_font(FONT_ID_SHINYPEABERRY),
		.text = text,
		.color = primary_pallet.main,
	};

	button->widget_interface->style_element->width = 16+al_get_text_width(button->font,button->text);
	button->widget_interface->style_element->height = 50;

	return 1;
}
