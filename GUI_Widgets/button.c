// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"

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

	al_draw_filled_rounded_rectangle(-half_width, -half_height, half_width, half_height, 10, 10, button->color);

	if (button->text)
		al_draw_text(button->font, al_map_rgb_f(1, 1, 1),
			0, -0.5 * al_get_font_line_height(button->font),
			ALLEGRO_ALIGN_CENTRE, button->text);

	al_draw_rounded_rectangle(-half_width, -half_height, half_width, half_height, 10, 10, al_map_rgb(0, 0, 0), 2);
}

static void mask(const struct widget_interface* const widget)
{
	const struct button* const button = (struct button*)widget->upcast;
	const double half_width = 0.5 * button->widget_interface->style_element->width;
	const double half_height = 0.5 * button->widget_interface->style_element->height;

	al_draw_filled_rounded_rectangle(-half_width, -half_height, half_width, half_height, 10, 10, al_map_rgb(72, 91, 122));
}

static void hover_start(struct widget_interface* const widget)
{
	struct button* const button = (struct button*)widget->upcast;

	button->color = al_map_rgb(0, 91, 122);
}

static void hover_end(struct widget_interface* const widget)
{
	struct button* const button = (struct button*)widget->upcast;

	button->color = al_map_rgb(72, 91, 122);
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

	*button = (struct button)
	{
		.widget_interface = widget_interface_new(L,button,&button_jump_table_entry),
		.font = test_font,
		.text = "test",
		.color = al_map_rgb(72, 91, 122),
	};

	button->widget_interface->style_element->width = 100;
	button->widget_interface->style_element->height = 50;

	return 1;
}
