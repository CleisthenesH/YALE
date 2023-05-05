// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include "widget_style_sheet.h"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include "allegro5/allegro_primitives.h"

#define WIDGET_TYPE slider

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

WG_DECL_DRAW
{
	WG_CAST_DRAW

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

WG_DECL_MASK
{
	WG_CAST_DRAW

	al_draw_filled_rounded_rectangle(-half_width, -half_height, half_width, half_height,
		primary_pallet.edge_radius, primary_pallet.edge_radius,
		al_map_rgb(255,255,255));
}	

WG_DECL(update)
{
	WG_CAST
	
	if (slider->state != UPDATE)
		 return;

	WG_DIMENSIONS
	double x = mouse_x;
	double y = mouse_y;
	widget_screen_to_local(slider->widget_interface, &x, &y);

	slider->progress = x / (2*(half_width - slider_padding))+  0.5;

	if (slider->progress < 0)
		slider->progress = 0;
	else if (slider->progress > 1)
		slider->progress = 1;
}

WG_DECL(left_click)
{
	WG_CAST
	slider->state = UPDATE;
}

WG_DECL(left_click_end)
{
	WG_CAST
	slider->state = IDLE;
}

WG_DECL(hover_start)
{
	WG_CAST

	if (slider->state == IDLE)
		slider->state = HOVER;
}

WG_DECL(hover_end)
{
	WG_CAST

	if (slider->state == HOVER)
		slider->state = IDLE;
}

WG_JMP_TBL
{
	.draw = draw,
	.mask = mask,
	.update = update,

	.left_click = left_click,
	.left_click_end = left_click_end,

	.hover_start = hover_start,
	.hover_end = hover_end,
};

WG_DECL_NEW
{
	WG_NEW
	{
		WG_NEW_HEADER,
		.start = 0,
		.end = 100,
		.progress = 0.2
	};

	WIDGET_MIN_DIMENSIONS(350, 25)

	return 1;
}