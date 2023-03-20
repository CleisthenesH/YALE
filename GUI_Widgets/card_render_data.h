// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// An object for holding all the data to render a card and card effects.
#pragma once

#include "renderer_interface.h"

struct card_render_data
{
	ALLEGRO_BITMAP* art;
	ALLEGRO_BITMAP* main;
	ALLEGRO_BITMAP* advantage;
	ALLEGRO_BITMAP* disadvantage;

	int level;
	char* name;
	char* text_box;
};

void card_render_draw(struct card_render_data*, struct render_interface*);
void card_render_mask(struct card_render_data*, struct render_interface*);