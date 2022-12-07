// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "style_element.h"
#include "thread_pool.h"

#include <stdio.h>
#include <math.h>
#include <allegro5/allegro_opengl.h>

static ALLEGRO_SHADER* style_shader;

struct style_element
{
	size_t used;
	size_t allocated;
	struct keyframe* keyframes;

	double foil_offset;

	struct keyframe current;
};

static struct style_element* list;
static size_t allocated;
static size_t used;

extern double current_timestamp;

void style_element_init()
{
	used = 0;
	allocated = 100;
	list = malloc(allocated * sizeof(struct style_element));

	style_shader = al_create_shader(ALLEGRO_SHADER_GLSL);

	if (!al_attach_shader_source_file(style_shader, ALLEGRO_VERTEX_SHADER, "style_vertex_shader.glsl"))
	{
		fprintf(stderr, "Failed to attach vertex shader.\n%s\n", al_get_shader_log(style_shader));
		return;
	}

	if (!al_attach_shader_source_file(style_shader, ALLEGRO_PIXEL_SHADER, "style_pixel_shader.glsl"))
	{
		fprintf(stderr, "Failed to attach pixel shader.\n%s\n", al_get_shader_log(style_shader));
		return;
	}

	if (!al_build_shader(style_shader))
	{
		fprintf(stderr,"Failed to build shader.\n%s\n", al_get_shader_log(style_shader));
		return;
	}

}

void style_element_predraw()
{
	al_use_shader(style_shader);
}

// TEST
struct style_element* style_element_new(size_t hint)
{
	if (allocated <= used)
	{
		const size_t new_cnt = 2 * allocated;

		struct style_element* memsafe_hande = realloc(list, new_cnt * sizeof(struct style_element));

		if (!memsafe_hande)
			return NULL;

		list = memsafe_hande;
		allocated = new_cnt;
	}

	struct style_element* style_element = list + used++;

	style_element->used = 0;
	style_element->allocated = hint;
	style_element->keyframes = malloc(hint * sizeof(struct keyframe));
	style_element->foil_offset = fmod(current_timestamp,100);

	return style_element;
}

static void style_element_update_work(struct style_element* style_element)
{
	// Clean old frames;
	size_t first_future_frame = 0;

	while (style_element->keyframes[first_future_frame].timestamp < current_timestamp &&
		first_future_frame < style_element->used)
		first_future_frame++;

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

void style_element_set(struct style_element* style_element, struct keyframe* set)
{
	style_element->used = 1;

	memcpy(&style_element->current, set, sizeof(struct keyframe));
	memcpy(style_element->keyframes, set, sizeof(struct keyframe));
}

struct keyframe* style_element_new_frame(struct style_element* style_element)
{
	// deal with updating timestamp is used == 1
	if (style_element->allocated <= style_element->used)
	{
		const size_t new_cnt = 2 * style_element->allocated;

		struct keyframe* memsafe_hande = realloc(style_element->keyframes, new_cnt*sizeof(struct keyframe));

		if (!memsafe_hande)
			return NULL;

		style_element->keyframes = memsafe_hande;
		style_element->allocated = new_cnt;
	}

	if (style_element->used == 1)
		style_element->keyframes[0].timestamp = current_timestamp;

	struct keyframe* output = style_element->keyframes + style_element->used;

	memcpy(output, output-1, sizeof(struct keyframe));
	style_element->used++;

	return output;
}

void style_element_apply(struct style_element* style_element)
{
	al_set_shader_float("saturate", style_element->current.saturate);
	al_set_shader_float("foil_effect", style_element->foil_offset + current_timestamp);

	ALLEGRO_TRANSFORM buffer;
	al_build_transform(&buffer, 
		style_element->current.x, style_element->current.y, 
		//1,1,
		style_element->current.sx, style_element->current.sy, 
		style_element->current.t);

	al_use_transform(&buffer);
}
