// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "rectangle.h"
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_opengl.h>

extern ALLEGRO_FONT* debug_font;

static void draw(const struct widget_interface* const widget)
{
	const struct rectangle* const rectangle = (struct rectangle*) widget->upcast;
	const double half_width = 0.5*rectangle->width;
	const double half_height = 0.5*rectangle->height;

	//glStencilMask(0x80);
	//glStencilFunc(GL_REPLACE, 0x80, 0x80);

	al_draw_filled_rectangle(-half_width, -half_height, half_width, half_height, rectangle->color);
	al_draw_textf(debug_font, al_map_rgb_f(0, 1, 0), 0, 0, ALLEGRO_ALIGN_CENTER, "%d", rectangle->cnt);
}

static void hover_start(struct widget_interface* const widget)
{
	struct rectangle* const rectangle = (struct rectangle*)widget->upcast;

	rectangle->color = al_map_rgb_f(1, 0, 0);
}

static void hover_end(struct widget_interface* const widget)
{
	struct rectangle* const rectangle = (struct rectangle*)widget->upcast;

	rectangle->color = al_map_rgb_f(1, 1,1);
}

static void left_click(struct widget_interface* const widget)
{
	struct rectangle* const rectangle = (struct rectangle*)widget->upcast;

	rectangle->cnt++;
}

static void right_click(struct widget_interface* const widget)
{
	struct rectangle* const rectangle = (struct rectangle*)widget->upcast;

	rectangle->cnt--;
}

struct rectangle* rectangle_new()
{
	struct rectangle* const rectangle = malloc(sizeof(struct rectangle));

	if (!rectangle)
		return NULL;

	*rectangle = (struct rectangle)
	{
		.color = al_map_rgb_f(1,1,1),
		.height = 32,
		.width = 100,
		.cnt = 0,
		.widget_interface = widget_interface_new(rectangle,draw,NULL,NULL,NULL)
	};

	rectangle->widget_interface->hover_start = hover_start;
	rectangle->widget_interface->hover_end = hover_end;
	rectangle->widget_interface->left_click = left_click;
	rectangle->widget_interface->right_click = right_click;

	return rectangle;
}

