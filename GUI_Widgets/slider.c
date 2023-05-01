// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include "widget_style_sheet.h"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include "allegro5/allegro_primitives.h"

extern ALLEGRO_FONT* test_font;
extern double mouse_x;
extern double mouse_y;

struct slider
{
	struct widget_interface* widget_interface;

	enum {
		IDLE,
		HOVER,
		UPDATE
	} state;

	bool update;
	double start, end, progress;
};

const double slider_padding = 16;

static ALLEGRO_COLOR holder_color(const struct slider* const slider)
{
	switch (slider->state)
	{
	default:
	case IDLE:
		return primary_pallet.edge;
	case HOVER:
		return primary_pallet.highlight;
	case UPDATE:
		return primary_pallet.activated;
	}
}

static void draw(const struct widget_interface* const widget)
{
	const struct slider* const slider = (struct slider*)widget->upcast;
	const double half_width = 0.5 * slider->widget_interface->style_element->width;
	const double half_height = 0.5 * slider->widget_interface->style_element->height;

	al_draw_filled_rounded_rectangle(-half_width, -half_height, half_width, half_height,
		primary_pallet.edge_radius, primary_pallet.edge_radius,
		primary_pallet.main);

	al_draw_line(-half_width + slider_padding, 0, half_width - slider_padding, 0,
		primary_pallet.edge, primary_pallet.edge_width);

	const double center = -half_width+slider_padding + slider->progress * 2 * (half_width-slider_padding);
	al_draw_filled_rectangle(center - 4, -4, center + 4, 4,
		holder_color(slider));

	al_draw_rounded_rectangle(-half_width, -half_height, half_width, half_height,
		primary_pallet.edge_radius, primary_pallet.edge_radius,
		primary_pallet.edge, primary_pallet.edge_width);
}

static void mask(const struct widget_interface* const widget)
{
	const struct slider* const slider = (struct slider*)widget->upcast;
	const double half_width = 0.5 * slider->widget_interface->style_element->width;
	const double half_height = 0.5 * slider->widget_interface->style_element->height;

	al_draw_filled_rounded_rectangle(-half_width, -half_height, half_width, half_height,
		primary_pallet.edge_radius, primary_pallet.edge_radius,
		al_map_rgb(255,255,255));
}	

static void update(const struct widget_interface* const widget)
{
	 struct slider* const slider = (struct slider*)widget->upcast;

	 if (slider->state != UPDATE)
		 return;

	double x = mouse_x;
	double y = mouse_y;
	widget_screen_to_local(slider->widget_interface, &x, &y);

	slider->progress = x / (slider->widget_interface->style_element->width -2*slider_padding)+  0.5;

	if (slider->progress < 0)
		slider->progress = 0;
	else if (slider->progress > 1)
		slider->progress = 1;
}

static void left_click(const struct widget_interface* const widget)
{
	struct slider* const slider = (struct slider*)widget->upcast;
	slider->state = UPDATE;
}

static void left_click_end(const struct widget_interface* const widget)
{
	struct slider* const slider = (struct slider*)widget->upcast;
	slider->state = IDLE;
}

static void hover_start(struct widget_interface* widget)
{
	struct slider* const slider = (struct slider*)widget->upcast;

	if (slider->state == IDLE)
		slider->state = HOVER;
}

static void hover_end(struct widget_interface* widget)
{
	struct slider* const slider = (struct slider*)widget->upcast;

	if (slider->state == HOVER)
		slider->state = IDLE;
}

static const struct widget_jump_table slider_jump_table_entry =
{
	.draw = draw,
	.mask = mask,
	.update = update,

	.left_click = left_click,
	.left_click_end = left_click_end,

	.hover_start = hover_start,
	.hover_end = hover_end,
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

	slider->widget_interface->style_element->width = 700;
	slider->widget_interface->style_element->height = 50;

	return 1;
}