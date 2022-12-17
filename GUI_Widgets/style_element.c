// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "style_element.h"
#include "thread_pool.h"

#include <stdio.h>
#include <math.h>
#include <allegro5/allegro_opengl.h>

static ALLEGRO_SHADER* predraw_shader;
static ALLEGRO_SHADER* postdraw_shader;

struct style_element_internal
{
	struct style_element;
	size_t used;
	size_t allocated;
	struct keyframe* keyframes;

	double variation;
};

static struct style_element_internal* list;
static size_t allocated;
static size_t used;

extern double current_timestamp;

static inline int make_predraw_shader()
{
	predraw_shader = al_create_shader(ALLEGRO_SHADER_GLSL);

	if (!al_attach_shader_source_file(predraw_shader, ALLEGRO_VERTEX_SHADER, "predraw_vertex_shader.glsl"))
	{
		fprintf(stderr, "Failed to attach predraw vertex shader.\n%s\n", al_get_shader_log(predraw_shader));
		return 0;
	}

	if(0)
	al_attach_shader_source(predraw_shader, ALLEGRO_VERTEX_SHADER, 
		al_get_default_shader_source(ALLEGRO_SHADER_GLSL, ALLEGRO_VERTEX_SHADER));

	if(0)
	al_attach_shader_source(predraw_shader, ALLEGRO_PIXEL_SHADER,
		al_get_default_shader_source(ALLEGRO_SHADER_GLSL, ALLEGRO_PIXEL_SHADER));

	if(0)
	printf(al_get_default_shader_source(ALLEGRO_SHADER_GLSL, ALLEGRO_PIXEL_SHADER));

	if(1)
	if (!al_attach_shader_source_file(predraw_shader, ALLEGRO_PIXEL_SHADER, "predraw_pixel_shader.glsl"))
	{
		fprintf(stderr, "Failed to attach predraw pixel shader.\n%s\n", al_get_shader_log(predraw_shader));
		return 0;
	}

	if (!al_build_shader(predraw_shader))
	{
		fprintf(stderr, "Failed to build predraw shader.\n%s\n", al_get_shader_log(predraw_shader));
		return 0;
	}

	return 1;
}

static inline int make_postdraw_shader()
{
	postdraw_shader = al_create_shader(ALLEGRO_SHADER_GLSL);

	if (!al_attach_shader_source_file(postdraw_shader, ALLEGRO_VERTEX_SHADER, "postdraw_vertex_shader.glsl"))
	{
		fprintf(stderr, "Failed to attach postdraw vertex shader.\n%s\n", al_get_shader_log(postdraw_shader));
		return 0;
	}

	if (!al_attach_shader_source_file(postdraw_shader, ALLEGRO_PIXEL_SHADER, "postdraw_pixel_shader.glsl"))
	{
		fprintf(stderr, "Failed to attach postdraw pixel shader.\n%s\n", al_get_shader_log(postdraw_shader));
		return 0;
	}

	if (!al_build_shader(postdraw_shader))
	{
		fprintf(stderr, "Failed to build postdraw shader.\n%s\n", al_get_shader_log(postdraw_shader));
		return 0;
	}

	return 1;
}

void style_element_init()
{
	used = 0;
	allocated = 100;
	list = malloc(allocated * sizeof(struct style_element_internal));

	make_predraw_shader();
	//make_postdraw_shader();
}

// TEST
struct style_element* style_element_new(size_t hint)
{
	if (allocated <= used)
	{
		const size_t new_cnt = 2 * allocated;

		struct style_element_internal* memsafe_hande = realloc(list, new_cnt * sizeof(struct style_element_internal));

		if (!memsafe_hande)
			return NULL;

		list = memsafe_hande;
		allocated = new_cnt;
	}

	struct style_element_internal* style_element = list + used++;

	style_element->used = 0;
	style_element->allocated = hint;
	style_element->keyframes = malloc(hint * sizeof(struct keyframe));
	style_element->variation = fmod(current_timestamp,100);

	return (struct style_element*) style_element;
}

static void style_element_update_work(struct style_element_internal* style_element)
{
	// Clean old frames;
	size_t first_future_frame = 0;

	while (style_element->keyframes[first_future_frame].timestamp < current_timestamp &&
		first_future_frame < style_element->used)
		first_future_frame++;

	// This makes it jittery maybe keep one a copy of the original and belend with that
	if (first_future_frame > 1)
	{
		const size_t step = first_future_frame - 1;

		for (size_t i = step; i < style_element->used; i++)
			style_element->keyframes[i - step] = style_element->keyframes[i];

		style_element->used -= step;

		if (style_element->used == 1)
		{
			style_element->current = style_element->keyframes[0];
			return;
		}
	}

	const double blend = (current_timestamp - style_element->keyframes[0].timestamp) / (style_element->keyframes[1].timestamp - style_element->keyframes[0].timestamp);

#define blend(X) style_element->current.X = blend * style_element->keyframes[1].X + (1 - blend) * style_element->keyframes[0].X
	blend(x);
	blend(y);
	blend(sx);
	blend(sy);
	blend(t);
	blend(saturate);
	blend(blender_color.r);
	blend(blender_color.g);
	blend(blender_color.b);
	blend(blender_color.a);
#undef blend
}

struct work_queue* style_element_update()
{
	struct work_queue* work_queue = work_queue_create();

	for (size_t i = 0; i < used; i++)
		if (list[i].used > 1)
			work_queue_push(work_queue, style_element_update_work, list + i);

	return work_queue;
}

void style_element_set(struct style_element* const style_element, struct keyframe* const set)
{
	struct style_element_internal* const internal = (struct style_element_internal* const) style_element;

	internal->used = 1;

	memcpy(&style_element->current, set, sizeof(struct keyframe));
	memcpy(internal->keyframes, set, sizeof(struct keyframe));
}

struct keyframe* style_element_new_frame(struct style_element* const style_element)
{
	struct style_element_internal* const internal = (struct style_element_internal* const) style_element;

	if (internal->allocated <= internal->used)
	{
		const size_t new_cnt = 2 * internal->allocated;

		struct keyframe* memsafe_hande = realloc(internal->keyframes, new_cnt*sizeof(struct keyframe));

		if (!memsafe_hande)
			return NULL;

		internal->keyframes = memsafe_hande;
		internal->allocated = new_cnt;
	}

	if (internal->used == 1)
		internal->keyframes[0].timestamp = current_timestamp;

	struct keyframe* output = internal->keyframes + internal->used;

	memcpy(output, output-1, sizeof(struct keyframe));
	internal->used++;

	return output;
}

void keyframe_build_transform(struct keyframe* const keyframe, ALLEGRO_TRANSFORM* const trans)
{
	al_build_transform(trans,
		keyframe->x, keyframe->y,
		keyframe->sx, keyframe->sy,
		keyframe->t);
}

void style_element_setup()
{
	al_use_shader(predraw_shader); 
	glDisable(GL_STENCIL_TEST);
	al_set_shader_float("current_timestamp", current_timestamp);
}

void style_element_predraw(const struct style_element* const style_element)
{
	struct style_element_internal* const internal = (struct style_element_internal* const)style_element;

	al_set_shader_float("saturate", style_element->current.saturate);
	al_set_shader_float("variation", internal->variation);

	ALLEGRO_TRANSFORM buffer;
	keyframe_build_transform(&style_element->current, &buffer);

	al_use_transform(&buffer);

	glDisable(GL_STENCIL_TEST);
}

// To handle the varity of effect and selection data I've implemented a very basic type system.

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

static inline struct selection_data* get_selection(struct effect_element* effect_element)
{
	struct effect_data* const effect_data = (struct effect_data* const) effect_element;
	return (struct selection_data* const)((char*)effect_element) + effect_data_size(effect_data->id);
}

void style_element_effect(
	const struct style_element* const style_element, 
	struct effect_element* effect_element)
{
	if (!effect_element)
	{
		al_set_shader_int("effect_id", EFFECT_ID_NULL);
		al_set_shader_int("selection_id", SELECTION_ID_FULL);
		return;
	}

	struct effect_data* const effect_data = (struct effect_data* const) effect_element;
	struct selection_data* const selection_data = get_selection(effect_element);

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

	/*
	switch (selection_data->id)
	{
	case SELECTION_ID_COLOR_BAND:
		((struct selection_color_band*)selection_data)->color = ;
		return;
	}
	*/
}

struct effect_element* effect_element_new(
	enum EFFECT_ID effect_id,
	enum SELECTION_ID selection_id)
{
	const size_t selection_block = selection_data_size(selection_id);
	const size_t effect_block = effect_data_size(effect_id);

	struct effect_element* const output = malloc(selection_block+ effect_block);
	if (!output)
		return NULL;

	struct effect_data* const effect_data = (struct effect_data* const) output;
	struct selection_data* const selection_data = get_selection(output);

	effect_data->id = effect_id;
	selection_data->id = selection_id;
	
	return output;
}

void effect_element_selection_color(struct effect_element* const element, ALLEGRO_COLOR color)
{
	struct selection_data* const selection_data = get_selection(element);

	switch (selection_data->id)
	{
	case SELECTION_ID_COLOR_BAND:
		((struct selection_color_band*)selection_data)->color = color;
		return;
	default:
		; // throw error
	}

}

void effect_element_selection_cutoff(struct effect_element* const element, double cutoff)
{
	struct selection_data* const selection_data = get_selection(element);

	switch (selection_data->id)
	{
	case SELECTION_ID_COLOR_BAND:
		((struct selection_color_band*)selection_data)->cutoff = cutoff;
		return;
	default:
		; // throw error
	}
}

void effect_element_point(struct effect_element* const element, double x, double y)
{
	struct effect_data* const effect_data = (struct effect_data*)element;

	switch (effect_data->id)
	{
	case EFFECT_ID_RADIAL_RGB:
		((struct effect_radial_rgb*) effect_data)->center_x = x;
		((struct effect_radial_rgb*) effect_data)->center_y = y;
		return;
	default:
		; //throw error
	}
}
