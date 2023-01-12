// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include "allegro5/allegro_primitives.h"

extern ALLEGRO_FONT* test_font;
extern double mouse_x;
extern double mouse_y;

struct slider
{
	struct widget_interface* widget_interface;

	bool update;
	double start, end, progress;
};

static void draw(const struct widget_interface* const widget)
{
	const struct slider* const slider = (struct slider*)widget->upcast;
	const double half_width = 0.5 * slider->widget_interface->style_element->width;
	const double half_height = 0.5 * slider->widget_interface->style_element->height;

	al_draw_line(-half_width, 0, half_width, 0, al_color_name("pink"), 2);

	const double center = -half_width + slider->progress * 2 * half_width;
	al_draw_filled_rectangle(center - 2, -2, center + 2, 2, al_color_name("pink"));
}

static void mask(const struct widget_interface* const widget)
{
	const struct slider* const slider = (struct slider*)widget->upcast;
	const double half_width = 0.5 * slider->widget_interface->style_element->width;

	const double center = -half_width + slider->progress * 2 * half_width;
	al_draw_filled_rectangle(center - 2, -2, center + 2, 2, al_color_name("white"));
}	

// TODO: create a "update on hover" method
static void update(const struct widget_interface* const widget)
{
	 struct slider* const slider = (struct slider*)widget->upcast;

	if (!slider->update)
		return;

	double x = mouse_x;
	double y = mouse_y;
	widget_screen_to_local(slider->widget_interface, &x, &y);

	slider->progress = x / slider->widget_interface->style_element->width +  0.5;

	if (slider->progress < 0)
		slider->progress = 0;
	else if (slider->progress > 1)
		slider->progress = 1;
}

static void left_click(const struct widget_interface* const widget)
{
	struct slider* const slider = (struct slider*)widget->upcast;
	slider->update = true;
}

static void left_click_end(const struct widget_interface* const widget)
{
	struct slider* const slider = (struct slider*)widget->upcast;
	slider->update = false;
}

static const struct widget_jump_table slider_jump_table_entry =
{
	.draw = draw,
	.mask = mask,
	.left_click = left_click,
	.left_click_end = left_click_end,
	.update = update,
};

int slider_new(lua_State* L)
{
	struct slider* const slider = malloc(sizeof(struct slider));

	if (!slider)
		return 0;

	*slider = (struct slider)
	{
		.widget_interface = widget_interface_new(L,slider,&slider_jump_table_entry),
		.start = 0,
		.end = 100,
		.progress = 0.2
	};

	slider->widget_interface->style_element->width = 100;
	slider->widget_interface->style_element->height = 50;

	return 1;
}