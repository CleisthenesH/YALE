// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "card.h"
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_opengl.h>

extern ALLEGRO_FONT* debug_font;
static ALLEGRO_BITMAP* alex;

static void draw(const struct widget_interface* const widget_interface)
{
	const struct card* const card = (const struct card*)widget_interface->upcast;

	glEnable(GL_STENCIL_TEST);	
	glStencilFunc(GL_ALWAYS, 1, 0x03);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	style_element_effect(widget_interface->style_element, 0);

	al_draw_filled_rounded_rectangle(-90, -127, 90, 127, 10, 10, al_color_name("forestgreen"));

	glStencilFunc(GL_ALWAYS, 2, 0x03);
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

	al_draw_filled_rounded_rectangle(-90+5, -127+5+10, 90-5, -10, 10, 10, al_map_rgb_f(1,1,1));

	glStencilFunc(GL_EQUAL, 2, 0x03);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	al_draw_scaled_bitmap(alex, 0, 0, 255, 256, -90+5, -127+5+10, 2*(90-5), 127-5, 0);

	glStencilFunc(GL_ALWAYS, 0, 0x03);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);


	al_draw_text(debug_font, al_map_rgb_f(0,0, 0), 0, -127, ALLEGRO_ALIGN_CENTER,  card->name);
	al_draw_text(debug_font, al_map_rgb_f(0,0, 0), 0, 20, ALLEGRO_ALIGN_CENTER,  card->effect);
	al_draw_rounded_rectangle(-90, -127, 90, 127, 10, 10, al_color_name("gold"), 2);
	al_draw_rounded_rectangle(-90 + 5, -127 + 5 + 10, 90 - 5, -10, 10, 10, al_color_name("gold"), 2);

	style_element_effect(widget_interface->style_element, 1);
	glStencilFunc(GL_EQUAL, 1, 0x03);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	al_draw_filled_rounded_rectangle(-90, -127, 90, 127, 10, 10, al_map_rgb_f(1, 1, 1));


	//al_draw_filled_circle(0, 0, 32, al_map_rgb_f(1, 0, 0));
}

static void mask(const struct widget_interface* const widget)
{
	al_draw_filled_rounded_rectangle(-90, -127, 90, 127, 10, 10, al_map_rgb_f(1, 1, 1));
}

struct card* card_new(const char* name)
{
	struct card* const card = malloc(sizeof(struct card));

	if (!card)
		return NULL;

	*card = (struct card)
	{
		.name = name,
		.artwork = alex ? alex : (alex = al_load_bitmap("res/alex.bmp")),
		.effect = "Cost: Effect",
		.widget_interface = widget_interface_new(card,draw,NULL,NULL,mask),
	};

	/*
	rectangle->widget_interface->hover_start = hover_start;
	rectangle->widget_interface->hover_end = hover_end;
	rectangle->widget_interface->left_click = left_click;
	rectangle->widget_interface->right_click = right_click;
	*/

	return card;

}