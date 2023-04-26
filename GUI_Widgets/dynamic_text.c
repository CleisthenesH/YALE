// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "dynamic_text.h"

#include <stdlib.h>
#include <string.h>

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

// Dynamic Text jumptable

static dynamic_text_draw(void* data, double timestamp)
{
	const struct dynamic_text* const dynamic_text = (const struct dynamic_text* const)data;
	al_draw_scaled_text(dynamic_text->font, al_map_rgb(255, 255, 255),
		dynamic_text->x, dynamic_text->y,
		16, dynamic_text->scale, 0, dynamic_text->text);

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
	.draw = dynamic_text_draw

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