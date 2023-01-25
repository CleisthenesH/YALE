// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "style_element.h"
#include "thread_pool.h"

// To handle the varity of effect and selection data I've implemented a very basic type system.

enum SELECTION_COMPONENTS
{
	SELECTION_COMPONENT_COLOR = 1,
	SELECTION_COMPONENT_CUTOFF = 2,
	SELECTION_COMPONENT_POINT = 4,
	
	SELECTION_COMPONENT_TOTAL = 3
};

enum EFFECT_COMPONENTS
{
	EFFECT_COMPONENT_POINT = 1,

	EFFECT_COMPONENT_TOTAL = 1,
};

struct material
{
	enum EFFECT_ID effect_id;
	enum SELECTION_ID selection_id;
	enum SELECTION_COMPONENTS selection_components;
	enum EFFECT_COMPONENTS effect_components;
};

static size_t selection_offset[SELECTION_COMPONENT_TOTAL] =
{
	sizeof(double)*3,
	sizeof(double),
	sizeof(double)*2
};

static size_t effect_offset[EFFECT_COMPONENT_TOTAL] =
{
	sizeof(double) * 3,
};

struct effect_data
{
	enum EFFECT_ID id;
};

struct selection_data
{
	enum SELECTION_ID id;
};

struct effect_radial_rgb // Not implemented yet
{
	struct effect_data;
	float center_x, center_y;
};

struct selection_color_band
{
	struct selection_data;
	ALLEGRO_COLOR color;
	float cutoff;
};

static inline size_t effect_data_size(enum EFFECT_ID id)
{
	switch (id)
	{
	default:
		return sizeof(struct effect_data);
	}

}

static inline size_t selection_data_size(enum SELECTION_ID id)
{
	switch (id)
	{
	case SELECTION_ID_COLOR_BAND:
		return sizeof(struct selection_color_band);
	default:
		return sizeof(struct selection_data);
	}
}

static inline struct selection_data* get_selection(struct material* material)
{
	struct effect_data* const effect_data = (struct effect_data* const)material;
	return (struct selection_data* const)((char*)material) + effect_data_size(effect_data->id);
}

struct material* material_new(
	enum EFFECT_ID effect_id,
	enum SELECTION_ID selection_id)
{
	const size_t selection_block = selection_data_size(selection_id);
	const size_t effect_block = effect_data_size(effect_id);

	struct material* const output = malloc(selection_block + effect_block);
	if (!output)
		return NULL;

	struct effect_data* const effect_data = (struct effect_data* const)output;
	struct selection_data* const selection_data = get_selection(output);

	effect_data->id = effect_id;
	selection_data->id = selection_id;

	return output;
}

void material_set_shader(const struct material* const material)
{
	if (!material)
	{
		al_set_shader_int("effect_id", EFFECT_ID_NULL);
		al_set_shader_int("selection_id", SELECTION_ID_FULL);
		return;
	}

	struct effect_data* const effect_data = (struct effect_data* const)material;
	struct selection_data* const selection_data = get_selection(material);

	al_set_shader_int("effect_id", effect_data->id);
	al_set_shader_int("selection_id", selection_data->id);

	al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

	switch (effect_data->id)
	{
	case EFFECT_ID_RADIAL_RGB:
		al_set_shader_float_vector("point", 2,
			(float[]) {
			((struct effect_radial_rgb*)effect_data)->center_x,
				((struct effect_radial_rgb*)effect_data)->center_y
		}, 1);
		return;
	}
}

void material_selection_color(struct material* const material, ALLEGRO_COLOR color)
{
	struct selection_data* const selection_data = get_selection(material);

	switch (selection_data->id)
	{
	case SELECTION_ID_COLOR_BAND:
		((struct selection_color_band*)selection_data)->color = color;
		return;
	default:
		; // throw error
	}

}

void material_selection_cutoff(struct material* const material, double cutoff)
{
	struct selection_data* const selection_data = get_selection(material);

	switch (selection_data->id)
	{
	case SELECTION_ID_COLOR_BAND:
		((struct selection_color_band*)selection_data)->cutoff = cutoff;
		return;
	default:
		; // throw error
	}
}

void material_effect_point(struct material* const material, double x, double y)
{
	struct effect_data* const effect_data = (struct effect_data*)material;

	switch (effect_data->id)
	{
	case EFFECT_ID_RADIAL_RGB:
		((struct effect_radial_rgb*)effect_data)->center_x = x;
		((struct effect_radial_rgb*)effect_data)->center_y = y;
		return;
	default:
		; //throw error
	}
}
