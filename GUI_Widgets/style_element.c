// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "style_element.h"
#include "thread_pool.h"

#include <stdio.h>
#include <math.h>
#include <allegro5/allegro_opengl.h>

extern double current_timestamp;

static ALLEGRO_SHADER* predraw_shader;
static ALLEGRO_SHADER* postdraw_shader;

// The tweener struct takes timestamped keypoints and blends them based on the current timestamp.
//	The main use case is to handle the varitables for the al_build_transform function (position, scale, angle).
//  Other use cases include a color or threshold for an effect.
// 
//	The structs memory is quite raw so it is only avavlible within this translation unit.
//	The user should be using the material interface.
// 
//	TODO: Include more complex tweening options like keypoint weight and tangent.

struct tweener
{
	size_t channels; //	The zeroth channel is the timestamp
	double* current;

	size_t used, allocated;
	double* keypoints; // could optimize better with flexable array member?

	bool looping;
	double looping_offset;

	// Callback when the path ends 
	void (*funct)(void*);
	void* data;
};

static struct tweener* tweeners_list;
static size_t tweeners_allocated;
static size_t tweeners_used;

struct tweener* tweener_new(size_t channels, size_t hint)
{
	if (tweeners_allocated <= tweeners_used)
	{
		const size_t new_cnt = 2 * tweeners_allocated;

		struct tweener* memsafe_hande = realloc(tweeners_list, new_cnt * sizeof(struct tweener));

		if (!memsafe_hande)
			return NULL;

		tweeners_list = memsafe_hande;
		tweeners_allocated = new_cnt;
	}

	if (hint == 0)
		hint = 1;

	struct tweener* tweener = tweeners_list + tweeners_used++;

	tweener->used = 0;
	tweener->allocated = hint;
	tweener->keypoints = malloc(hint * (channels + 1) * sizeof(double));
	tweener->channels = channels;
	tweener->current = malloc(channels * sizeof(double));
	tweener->looping = false;
	tweener->funct = NULL;
	tweener->data = NULL;

	return (struct tweener*)tweener;
}

static inline void tweener_blend_nonlooping(struct tweener* tweener)
{
	// Clean old frames;
	size_t first_future_frame = 0;

	while (tweener->keypoints[first_future_frame * (tweener->channels + 1)] < current_timestamp &&
		first_future_frame < tweener->used)
		first_future_frame++;

	if (first_future_frame > 1)
	{
		const size_t step = first_future_frame - 1;

		for (size_t i = step; i < tweener->used; i++)
			for (size_t j = 0; j <= tweener->channels; j++)
				tweener->keypoints[(i - step) * (tweener->channels + 1) + j] = tweener->keypoints[i * (tweener->channels + 1) + j];

		tweener->used -= step;

		if (tweener->used == 1)
		{
			memcpy(tweener->current, tweener->keypoints + 1, tweener->channels * sizeof(double));

			if (tweener->funct)
				tweener->funct(tweener->data);

			return;
		}
	}

	const double blend = (current_timestamp - tweener->keypoints[0]) / (tweener->keypoints[tweener->channels + 1] - tweener->keypoints[0]);

	for (size_t i = 0; i < tweener->channels; i++)
		tweener->current[i] = blend * tweener->keypoints[i + 2 + tweener->channels] + (1 - blend) * tweener->keypoints[i + 1];
}

static inline void tweener_blend_looping(struct tweener* tweener)
{
	// TODO: optimize this function

	double loop_time = tweener->keypoints[(tweener->used - 1) * (tweener->channels + 1)] - tweener->keypoints[0] + tweener->looping_offset; // can delay calculating this
	size_t loops = 0;
	size_t idx = 0; // maybe keep this index between calls

	while (tweener->keypoints[idx * (tweener->channels + 1)] <= current_timestamp - ((double)loops) * loop_time)
		if (++idx == tweener->used)
			loops++, idx = 0;

	// adjust timestamps to proper range
	for (size_t i = 0; i < tweener->used; i++)
		tweener->keypoints[i * (tweener->channels + 1)] += ((double)loops) * loop_time;

	//
	const size_t end_idx = idx * (tweener->channels + 1);
	const size_t start_idx = (tweener->channels + 1) * ((idx != 0) ? (idx - 1) : tweener->used - 1);

	const double blend = (current_timestamp - tweener->keypoints[start_idx]) / (tweener->keypoints[end_idx] - tweener->keypoints[start_idx]);

	for (size_t i = 0; i < tweener->channels; i++)
		tweener->current[i] = blend * tweener->keypoints[i + end_idx + 1] + (1 - blend) * tweener->keypoints[i + start_idx + 1];
}

static void tweener_blend_keypoints(struct tweener* tweener)
{
	if (tweener->looping)
		tweener_blend_looping(tweener);
	else
		tweener_blend_nonlooping(tweener);
}

struct work_queue* tweener_update()
{
	struct work_queue* work_queue = work_queue_create();

	// optimize this by doing the looping check before appding the work item
	for (size_t i = 0; i < tweeners_used; i++)
		if (tweeners_list[i].used > 1)
			work_queue_push(work_queue, tweener_blend_keypoints, tweeners_list + i);

	return work_queue;
}

static void tweener_set(struct tweener* const tweener, double* keypoint)
{
	tweener->used = 1;

	memcpy(tweener->current, keypoint, tweener->channels * sizeof(double));
	memcpy(tweener->keypoints + 1, keypoint, tweener->channels * sizeof(double));
}

static double* tweener_new_point(struct tweener* tweener)
{
	if (tweener->allocated <= tweener->used)
	{
		const size_t new_cnt = 2 * tweener->allocated;

		double* memsafe_hande = realloc(tweener->keypoints, new_cnt * (tweener->channels + 1) * sizeof(double));

		if (!memsafe_hande)
			return NULL;

		tweener->keypoints = memsafe_hande;
		tweener->allocated = new_cnt;
	}

	if (tweener->used == 1)
		tweener->keypoints[0] = current_timestamp;

	double* output = tweener->keypoints + tweener->used * (tweener->channels + 1);

	memcpy(output, output - tweener->channels - 1, sizeof(double) * (tweener->channels + 1));
	tweener->used++;

	return output;
}

static void tweener_enter_loop(struct tweener* tweener, double loop_offset)
{
	tweener->looping = true;
	tweener->looping_offset = loop_offset;
}

static void tweener_interupt(struct tweener* const tweener)
{
	if (tweener->used > 1)
		tweener_blend_keypoints(tweener);

	tweener->looping = false;

	tweener_set(tweener, tweener->current);
}

static double* tweener_destination(struct tweener* const tweener)
{
	return tweener->keypoints + (tweener->used - 1) * (tweener->channels + 1);
}

static void tweener_set_callback(struct tweener* const tweener, void (*funct)(void*), void* data)
{
	tweener->funct = funct;
	tweener->data = data;
}

// Not tested
static void tweener_plot(struct tweener* const tweener,
	void (*funct)(double timestamp, double* output, void* udata), void* udata,
	double start, double end, size_t steps)
{
	const double step_size = (end - start) / ((double)steps - 1);
	for (size_t i = 0; i < steps; i++)
	{
		const double timestamp = start + step_size * ((double)i);
		const double* new_point = tweener_new_point(tweener);

		funct(timestamp, new_point, udata);
	}
}

struct style_element_internal
{
	struct style_element;
	struct tweener* keyframe_tweener;
	double variation;
};

static struct style_element_internal* list;
static size_t allocated;
static size_t used;

static inline int make_shader()
{
	predraw_shader = al_create_shader(ALLEGRO_SHADER_GLSL);

	if (!al_attach_shader_source_file(predraw_shader, ALLEGRO_VERTEX_SHADER, "shaders/style_element_vertex_shader.glsl"))
	{
		fprintf(stderr, "Failed to attach predraw vertex shader.\n%s\n", al_get_shader_log(predraw_shader));
		return 0;
	}

	if (!al_attach_shader_source_file(predraw_shader, ALLEGRO_PIXEL_SHADER, "shaders/style_element_pixel_shader.glsl"))
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

void style_element_init()
{
	used = 0;
	allocated = 100;
	list = malloc(allocated * sizeof(struct style_element_internal));

	tweeners_used = 0;
	tweeners_allocated = 100;
	tweeners_list = malloc(allocated * sizeof(struct tweener));

	make_shader();
}

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

	if (hint < 1)
		hint = 1;

	style_element->keyframe_tweener = tweener_new(5, hint);
	style_element->variation = fmod(current_timestamp, 100);
	style_element->width = 0;
	style_element->height = 0;

	return (struct style_element*)style_element;
}

void style_element_enter_loop(struct style_element* const style_element, double looping_offset)
{
	struct style_element_internal* const internal = (struct style_element_internal* const)style_element;
	tweener_enter_loop(internal->keyframe_tweener, looping_offset);
}

static void style_element_update_work(struct style_element_internal* style_element)
{
	double* keypoint = style_element->keyframe_tweener->current;

	style_element->current.x = keypoint[0];
	style_element->current.y = keypoint[1];
	style_element->current.sx = keypoint[2];
	style_element->current.sy = keypoint[3];
	style_element->current.timestamp = keypoint[4];
}

struct work_queue* style_element_update()
{
	struct work_queue* work_queue = work_queue_create();

	for (size_t i = 0; i < used; i++)
		//if (tweeners_list[i].used > 1)
		work_queue_push(work_queue, style_element_update_work, list + i);

	return work_queue;
}

void style_element_set(struct style_element* const style_element, struct keyframe* const set)
{
	struct style_element_internal* const internal = (struct style_element_internal* const)style_element;
	struct tweener* const tweener = internal->keyframe_tweener;

	tweener_set(tweener, (double[]) { set->x, set->y, set->sx, set->sy, set->theta });

	memcpy(&style_element->current, set, sizeof(struct keyframe));  // maybe can be optimized out
}

void style_element_callback(struct sytle_element* const style_element, void (*funct)(void*), void* data)
{
	struct style_element_internal* const internal = (struct style_element_internal* const)style_element;
	struct tweener* const tweener = internal->keyframe_tweener;

	tweener_set_callback(tweener, funct, data);
}

void style_element_push_keyframe(struct style_element* const style_element, struct keyframe* frame)
{
	struct style_element_internal* const internal = (struct style_element_internal* const)style_element;
	double* new_point = tweener_new_point(internal->keyframe_tweener);

	new_point[0] = frame->timestamp;
	new_point[1] = frame->x;
	new_point[2] = frame->y;
	new_point[3] = frame->sx;
	new_point[4] = frame->sy;
	new_point[5] = frame->theta;
}

void style_element_copy_destination(struct style_element* const style_element, struct keyframe* keyframe)
{
	struct style_element_internal* const internal = (struct style_element_internal* const)style_element;

	double* keypoints = tweener_destination(internal->keyframe_tweener);

	keyframe->timestamp = keypoints[0];
	keyframe->x = keypoints[1];
	keyframe->y = keypoints[2];
	keyframe->sx = keypoints[3];
	keyframe->sy = keypoints[4];
	keyframe->theta = keypoints[5];
}

// Align the tweener's current with the style_element's current
void style_element_align_tweener(struct style_element* const style_element)
{
	style_element_set(style_element, &style_element->current);
}

void style_element_interupt(struct style_element* const style_element)
{
	struct style_element_internal* const internal = (struct style_element_internal* const)style_element;

	tweener_interupt(internal->keyframe_tweener);

	internal->current.x = internal->keyframe_tweener->current[0];
	internal->current.y = internal->keyframe_tweener->current[1];
	internal->current.sx = internal->keyframe_tweener->current[2];
	internal->current.sy = internal->keyframe_tweener->current[3];
	internal->current.theta = internal->keyframe_tweener->current[4];
}

void keyframe_build_transform(const struct keyframe* const keyframe, ALLEGRO_TRANSFORM* const trans)
{
	al_build_transform(trans,
		keyframe->x, keyframe->y,
		keyframe->sx, keyframe->sy,
		keyframe->theta);
}

void keyframe_default(struct keyframe* const keyframe)
{
	*keyframe = (struct keyframe)
	{
		.sx = 1,
		.sy = 1,
	};
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

	//al_set_shader_float("saturate", style_element->current.saturate);
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

static inline struct selection_data* get_selection(struct material* material)
{
	struct effect_data* const effect_data = (struct effect_data* const)material;
	return (struct selection_data* const)((char*)material) + effect_data_size(effect_data->id);
}

void style_element_apply_material(
	const struct style_element* const style_element,
	struct material* material)
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

void material_selection_color(struct material* const element, ALLEGRO_COLOR color)
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

void material_selection_cutoff(struct material* const element, double cutoff)
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

void material_effect_point(struct material* const element, double x, double y)
{
	struct effect_data* const effect_data = (struct effect_data*)element;

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
