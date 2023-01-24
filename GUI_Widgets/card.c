// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "card.h"
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_opengl.h>

extern ALLEGRO_FONT* debug_font;
extern double mouse_x, mouse_y;

static ALLEGRO_BITMAP* alex;

extern lua_State* main_lua_state;

static struct material* null_effect;
static struct material* plain_foil;
static struct material* rgb_radial;
static struct material* color_pick;

static void draw(const struct widget_interface* const widget_interface)
{
	const struct card* const card = (const struct card*)widget_interface->upcast;

	glEnable(GL_STENCIL_TEST);	

	// Draw card background
	glStencilFunc(GL_ALWAYS, 1, 0x07);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	style_element_apply_material(widget_interface->style_element, plain_foil);
	al_draw_filled_rounded_rectangle(-90, -127, 90, 127, 10, 10, al_color_name("forestgreen"));

	// Stencil the art zone
	glStencilFunc(GL_ALWAYS, 2, 0x07);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	style_element_apply_material(widget_interface->style_element, NULL);

	al_draw_filled_rounded_rectangle(-90+5, -127+5+10, 90-5, -10, 10, 10, al_map_rgb_f(1,1,1));

	/*
	// Draw the art
	glStencilFunc(GL_EQUAL, 2, 0x07);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	al_draw_scaled_bitmap(alex, 0, 0, 255, 256, -90+5, -127+5+10, 2*(90-5), 127-5, 0);
	*/

	// Draw other boarders
	glStencilFunc(GL_ALWAYS, 3, 0x07);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	effect_element_point(rgb_radial, mouse_x, mouse_y);
	style_element_apply_material(widget_interface->style_element, rgb_radial);

	al_draw_text(debug_font, al_map_rgb_f(0, 0, 0), 0, -127, ALLEGRO_ALIGN_CENTER, card->name);
	al_draw_text(debug_font, al_map_rgb_f(0, 0, 0), 0, 20, ALLEGRO_ALIGN_CENTER, card->effect);
	al_draw_rounded_rectangle(-90, -127, 90, 127, 10, 10, al_color_name("gold"), 2);
	al_draw_rounded_rectangle(-90 + 5, -127 + 5 + 10, 90 - 5, -10, 10, 10, al_color_name("gold"), 2);

	al_draw_textf(debug_font, al_map_rgb_f(0, 0, 0), 0, 20, ALLEGRO_ALIGN_CENTER, "%f,%f", mouse_x,mouse_y);

	// Pick out the black in the art
	glStencilFunc(GL_EQUAL, 2, 0x07);
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	effect_element_point(color_pick, mouse_x, mouse_y);
	style_element_apply_material(widget_interface->style_element, color_pick);

	al_draw_scaled_bitmap(alex, 0, 0, 255, 256, -90 + 5, -127 + 5 + 10, 2 * (90 - 5), 127 - 5, 0);

	/*
	// Background foil stripe
	style_element_effect(widget_interface->style_element, plain_foil);
	glStencilFunc(GL_EQUAL, 1, 0x07);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	al_draw_filled_rounded_rectangle(-90, -127, 90, 127, 10, 10, al_map_rgb_f(1, 1, 1));



	// Highlight

	glStencilFunc(GL_EQUAL, 3, 0x07);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	al_draw_scaled_bitmap(alex, 0, 0, 255, 256, -90 -5, -127-5, 2 * (90+5), (127+5) *2, 0);	
	*/
}

static void mask(const struct widget_interface* const widget)
{
	al_draw_filled_rounded_rectangle(-90, -127, 90, 127, 10, 10, al_map_rgb_f(1, 1, 1));
}

void card_new(lua_State* L)
{
	const char* name = "Alex";
	struct card* const card = malloc(sizeof(struct card));

	if (!card)
		return NULL;

	*card = (struct card)
	{
		.name = name,
		.artwork = alex ? alex : (alex = al_load_bitmap("res/alex.bmp")),
		.effect = (char*) "Cost: Effect",
		.widget_interface = widget_interface_new(L,card,draw,NULL,NULL,mask,NULL),
	};
	
	if (!null_effect)
	{
		plain_foil = effect_element_new(EFFECT_ID_PLAIN_FOIL,SELECTION_ID_FULL);
		rgb_radial = effect_element_new(EFFECT_ID_RADIAL_RGB,SELECTION_ID_FULL);
		color_pick = effect_element_new(EFFECT_ID_RADIAL_RGB,SELECTION_ID_COLOR_BAND);

		effect_element_selection_color(color_pick, al_map_rgb(0, 0, 0));
		effect_element_selection_cutoff(color_pick, 0.1);
	}

	luaL_getmetatable(L, "card_mt");
	lua_setmetatable(L, -2);

	return 1;
}

int card_make_metatable(lua_State* L)
{

	return 1;
}