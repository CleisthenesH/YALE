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

	double foil_offset;
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
	make_postdraw_shader();
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
	style_element->foil_offset = fmod(current_timestamp,100);

	style_element->effect_flags = EFFECT_NULL;

	style_element->stencil_effects[0] = FOIL_NULL;
	style_element->stencil_effects[1] = FOIL_NULL;
	style_element->stencil_effects[2] = FOIL_NULL;
	style_element->stencil_effects[3] = FOIL_NULL;

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

void style_element_build_transform(struct style_element* const style_element,ALLEGRO_TRANSFORM* trans)
{
	al_build_transform(trans,
		style_element->current.x, style_element->current.y,
		style_element->current.sx, style_element->current.sy,
		style_element->current.t);
}

// Call before srawing
void style_element_setup()
{
	al_use_shader(predraw_shader); // maybe keep track of what shader	

}

void style_element_predraw(const struct style_element* const style_element)
{
	struct style_element_internal* const internal = (struct style_element_internal* const)style_element;

	al_set_shader_float("saturate", style_element->current.saturate);
	al_set_shader_float("variation", ((struct style_element_internal*)style_element)->foil_offset + current_timestamp);

	ALLEGRO_TRANSFORM buffer;
	style_element_build_transform(style_element, &buffer);

	al_use_transform(&buffer);
}

void style_element_effect(const struct style_element* const style_element, int effect)
{
	if(effect == 0)
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
	else
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);

	al_set_shader_int("effect", effect);
}

void style_element_prefoiling(const struct style_element* const style_element)
{
	al_use_shader(postdraw_shader);



	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xF0);


}
