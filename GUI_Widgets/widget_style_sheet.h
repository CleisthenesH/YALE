// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>

struct GUI_pallet
{
	ALLEGRO_COLOR main;
	ALLEGRO_COLOR recess;
	ALLEGRO_COLOR highlight;

	ALLEGRO_COLOR edge;
	double edge_width, edge_radius;

	ALLEGRO_COLOR activated;
	ALLEGRO_COLOR deactivated;
};

struct GUI_pallet primary_pallet, secondary_pallet;

ALLEGRO_FONT* primary_font;
