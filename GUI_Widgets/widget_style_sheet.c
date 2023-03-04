// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_style_sheet.h"

#include <allegro5/allegro_color.h>
#include "resource_manager.h"

ALLEGRO_COLOR primary_background;
ALLEGRO_COLOR primary_highlight;

ALLEGRO_COLOR secondary_background;
ALLEGRO_COLOR secondary_highlight;

ALLEGRO_FONT* primary_font;

void widget_style_sheet_init()
{
	primary_background = al_color_name("LightSteelBlue");
	primary_highlight = al_color_name("LightGray");

	secondary_background = al_color_name("LightSlateGrey");
	secondary_highlight = al_color_name("DimGrey");

	primary_font = resource_manager_font(FONT_ID_SHINYPEABERRY);
}
