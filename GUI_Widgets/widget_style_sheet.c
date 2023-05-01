// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_style_sheet.h"

#include <allegro5/allegro_color.h>
#include "resource_manager.h"

struct GUI_pallet primary_pallet, secondary_pallet;

ALLEGRO_FONT* primary_font;

void widget_style_sheet_init()
{
	primary_pallet = (struct GUI_pallet)
	{
		.main = al_map_rgb(72, 91, 122), 
		.highlight = al_map_rgb(114,136,173), 
		.recess = al_map_rgb(0x6B,0x81,0x8C),

		.edge = al_map_rgb(0,0,0),
		.edge_radius = 10,
		.edge_width = 2,

		.activated = al_color_name("white"),
		.deactivated = al_color_name("grey")
	};

	secondary_pallet = primary_pallet;

	primary_font = resource_manager_font(FONT_ID_SHINYPEABERRY);
}
