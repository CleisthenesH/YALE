// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include "resource_manager.h"
#include "widget_style_sheet.h"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include "allegro5/allegro_primitives.h"

#define WIDGET_TYPE button

struct button
{
	struct widget_interface* widget_interface;
	char* text;
	ALLEGRO_FONT* font;
	ALLEGRO_COLOR color;
};

WG_DECL_DRAW
{
	WG_CAST_DRAW

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

WG_DECL_MASK
{
	WG_CAST_DRAW

	al_draw_filled_rounded_rectangle(-half_width, -half_height, half_width, half_height,
		primary_pallet.edge_radius, primary_pallet.edge_radius,
		al_map_rgb(255, 255, 255));
}

WG_DECL(hover_start)
{
	WG_CAST

	button->color = primary_pallet.highlight;
}

WG_DECL(hover_end)
{
	WG_CAST

	button->color = primary_pallet.main;
}

WG_JMP_TBL
{
	.draw = draw,

	// TODO: Investigate why seting mask to draw causes such a strange error. 
	// Shouldn't it all be set to a uniform color by the shader?
	.mask = mask,
	.hover_start = hover_start,
	.hover_end = hover_end,
};

WG_DECL_NEW
{
	char* text = NULL;

	if (lua_istable(L, -1))
	{
		lua_getfield(L, -1, "text");

		if (lua_isstring(L, -1))
		{
			size_t text_len;
			const char* buffer = lua_tolstring(L, -1, &text_len);

			text = malloc(sizeof(char) * (text_len + 1));

			if (text)
				strcpy_s(text, text_len + 1, buffer);
			else
				text = "Error Placeholder";
		}
		else
		{
			text = "Placeholder";
		}

		lua_pop(L, 1);
	}

	WG_NEW
	{
		WG_NEW_HEADER,
		.font = resource_manager_font(FONT_ID_SHINYPEABERRY),
		.text = text,
		.color = primary_pallet.main,
	};

	WIDGET_MIN_DIMENSIONS(8 + 0.5 * al_get_text_width(button->font, button->text), 25)

	return 1;
}
