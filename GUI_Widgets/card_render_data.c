// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "card_render_data.h"
#include "resource_manager.h"

#include "allegro5/allegro_primitives.h"

const float card_half_width = 90;
const float card_half_height = 191;
const float class_icon = 50;
const float advantage_icon = 40;

extern void al_draw_scaled_text(ALLEGRO_FONT* font, ALLEGRO_COLOR color, float x, float y, float dy, float scale, int flag, const char* text);

/* RAW EXPORT
const struct
{
	const float x, y, w, h;
} coordinates[] = {
	{59.28,133.57,58.23,58.23}, //class holder
	{61.93,136.86,50.95,50.95}, //class icon
	{67.05,60.43,386.17,538.41}, //background
	{96.79,98.15,326.86,268.35}, //art
	{113.71,357.64,326.39,41.09}, //name
	{92.22,396.98,334.86,163.43}, //textbox
	{50.28,550.2,58.23,58.23}, //left holder
	{407.63,550.2,58.23,58.23}, //right holder
	{62.77,552.84,50.95,50.95}, //left icon
	{412.15,552.84,50.95,50.95}, //right icon
	{48,48,78.86,78.86}, //level holder

};

int main()
{
	const float offset_x = -67.05 - 0.5 * (386.17);
	const float offset_y = -60.43 - 0.5 * (538.41);
	for (size_t i = 0; i < 11; i++)
		printf("{%f,%f,%f,%f},\n",
			coordinates[i].x + offset_x,
			coordinates[i].y + offset_y,
			coordinates[i].x + offset_x + coordinates[i].w,
			coordinates[i].y + offset_y + coordinates[i].h
			);
}
*/

const struct
{
	const float x0, y0, x1, y1;
} coordinates[] = {
	{-200.855011,-196.065002,-142.625015,-137.835007},
	{-198.205017,-192.775009,-147.255020,-141.825012},
	{-193.085007,-269.205017,193.085007,269.204956},
	{-163.345001,-231.485016,163.514984,36.864990},
	{-146.425018,28.005005,179.964996,69.095001},
	{-167.915009,67.345001,166.944977,230.774994},
	{-209.855011,220.565002,-151.625015,278.795013},
	{147.494995,220.565002,205.724991,278.795013},
	{-197.365005,223.205017,-146.415009,274.155029},
	{152.014984,223.205017,202.964981,274.155029},
	{-212.135010,-281.635010,-133.275009,-202.775009},
};

static inline void rounded(size_t i, const char* color, const char* second_color)
{
	al_draw_filled_rounded_rectangle(
		coordinates[i].x0, coordinates[i].y0,
		coordinates[i].x1, coordinates[i].y1,
		5, 5,
		al_color_name(color)
	);

	al_draw_rounded_rectangle(
		coordinates[i].x0, coordinates[i].y0,
		coordinates[i].x1, coordinates[i].y1,
		5, 5,
		al_color_name(second_color),
		5
	);
}

void card_render_draw(struct card_render_data* card, struct render_interface* element)
{
	rounded(2, "darkorchid","goldenrod");

	al_draw_scaled_bitmap(resource_manager_character_art(CHARACTER_ART_ID_SCHOLAR),
		0, 0, 512, 512,
		coordinates[3].x0, coordinates[3].y0,
		coordinates[3].x1 - coordinates[3].x0, coordinates[3].y1 - coordinates[3].y0,
		0);	



	al_draw_rectangle(
		coordinates[3].x0, coordinates[3].y0,
		coordinates[3].x1, coordinates[3].y1,
		al_color_name("goldenrod"), 5);

	rounded(0, "dimgray","black");

	al_draw_scaled_bitmap(resource_manager_icon(1861),
		0, 0, 512, 512,
		coordinates[1].x0, coordinates[1].y0,
		coordinates[1].x1 - coordinates[1].x0, coordinates[1].y1 - coordinates[1].y0,
		0);

	rounded(5, "blanchedalmond","goldenrod");

	rounded(4, "dimgray", "black");
	rounded(6, "dimgray", "black");
	rounded(7, "dimgray", "black");
	rounded(10, "dimgray", "black");

	al_draw_scaled_text(resource_manager_font(FONT_ID_WHITEPEABERRYOUTLINE),
		al_map_rgb(255, 255, 255),
		0.5 * (coordinates[10].x0 + coordinates[10].x1),
		0.5 * (coordinates[10].y0 + coordinates[10].y1),
		16, 3, 0,
		"7");
}

void card_render_mask(struct card_render_data* card, struct render_interface* element)
{

}
