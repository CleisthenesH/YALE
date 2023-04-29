// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "dynamic_text.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>


#include <allegro5/allegro.h>
#include "allegro5/allegro_font.h"

extern void al_draw_scaled_text(ALLEGRO_FONT* font, ALLEGRO_COLOR color, float x, float y, float dy, float scale, int flag, const char* text);

struct dynamic_text
{
	ALLEGRO_FONT* font;
	enum dynamic_text_animation animation_id;
	const char* text; // TODO: Flexable memeber array?
	double scale;
	double x, y;
};

// Utility

static void dynamic_text_build_transform(const struct dynamic_text* const dynamic_text, ALLEGRO_TRANSFORM* trans)
{
	const double x = dynamic_text->x;
	const double y = dynamic_text->y;
	const double dy = 16; // dynamic_text->dy;
	const double scale = dynamic_text->scale;

	al_build_transform(trans,
		x, y - dy * scale,
		scale, scale, 0);
}

// Animations 

static void dynamic_text_animation_still(const struct dynamic_text* const dynamic_text)
{
	al_draw_scaled_text(dynamic_text->font, al_map_rgb(255, 255, 255),
		dynamic_text->x, dynamic_text->y,
		16, dynamic_text->scale, 0, dynamic_text->text);
}

static void dynamic_text_animation_worm_(const struct dynamic_text* const dynamic_text, double timestamp)
{
	const ALLEGRO_TRANSFORM* const buffer = al_get_current_transform();
	ALLEGRO_TRANSFORM tmp;

	dynamic_text_build_transform(dynamic_text, &tmp);
	al_compose_transform(&tmp, buffer);
	al_use_transform(&tmp);

	al_draw_text(dynamic_text->font,
		al_map_rgb(255, 255, 255),
		0, 10 * sin(timestamp),
		ALLEGRO_ALIGN_CENTRE,
		dynamic_text->text);

	al_use_transform(buffer);
}

static void dynamic_text_animation_worm(const struct dynamic_text* const dynamic_text, double timestamp)
{
	
	char* buffer = dynamic_text->text;
	float xadvance = 0;

	while (*buffer)
	{
		al_draw_glyph(dynamic_text->font,al_map_rgb(255,255,255), 
			xadvance, 16*sin(timestamp+xadvance), *buffer);

		// No kerning, me and my homies hate kerning.
		xadvance += al_get_glyph_width(dynamic_text->font, *buffer);
		buffer++;
	}
}

static void dynamic_text_animation_wave(const struct dynamic_text* const dynamic_text, double timestamp)
{

	char* buffer = dynamic_text->text;
	float xadvance = 0;

	while (*buffer)
	{
		al_draw_glyph(dynamic_text->font, al_map_rgb(255, 255, 255),
			xadvance, 16 * sin(timestamp + xadvance), *buffer);

		// No kerning, me and my homies hate kerning.
		xadvance += al_get_glyph_width(dynamic_text->font, *buffer);
		buffer++;
	}
}

// Dynamic Text jumptable

static dynamic_text_draw(void* data, double timestamp)
{
	const struct dynamic_text* const dynamic_text = (const struct dynamic_text* const)data;

	switch (dynamic_text->animation_id)
	{
	default:
	case DYNAMIC_TEXT_STILL:
		dynamic_text_animation_still(dynamic_text);
		break;

	case DYNAMIC_TEXT_WORM:
		dynamic_text_animation_worm(dynamic_text, timestamp);
		break;

	case DYNAMIC_TEXT_WAVE:
		dynamic_text_animation_wave(dynamic_text, timestamp);
		break;
	}
}

static dynamic_text_update(void* data, double timestamp)
{

}

static dynamic_text_gc(void* data)
{
	const struct dynamic_text* const dynamic_text = (const struct dynamic_text* const)data;

	free(dynamic_text->text);
	free(dynamic_text);
}

const struct particle_jumptable dynamic_text_jumptable =
{
	.draw = dynamic_text_draw,
	.gc = dynamic_text_gc
};

// Public interface

void dynamic_text_new(struct particle_bin* bin,
	const char* text,
	double x, double y, double scale,
	enum font_id font_id,
	enum dynamic_text_animation animation_id,
	double duration)
{
	struct dynamic_text* dynamic_text = malloc(sizeof(struct dynamic_text));

	if (!dynamic_text)
		return;

	*dynamic_text = (struct dynamic_text) {
		.font = resource_manager_font(font_id),
		.animation_id = animation_id,
		.scale = scale,
		.x = x,
		.y = y
	};

	dynamic_text->text = malloc(sizeof(char) * (strlen(text)+1));

	if (!dynamic_text->text)
	{
		free(dynamic_text);
		return;
	}

	strcpy_s(dynamic_text->text, strlen(text) + 1, text);

	particle_bin_append(bin, &dynamic_text_jumptable, dynamic_text, duration);
}