// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "style_element.h"
#include "thread_pool.h"
#include "tweener.h"

#include <stdio.h>
#include <math.h>
#include <allegro5/allegro_opengl.h>

extern double current_timestamp;
extern void tweener_init();
extern struct work_queue* tweener_update();
extern void material_set_shader(const struct material* const);

static ALLEGRO_SHADER* predraw_shader;
static ALLEGRO_SHADER* postdraw_shader;

struct particle
{
	void (*draw)(double);
	size_t seed;
	double start_timestamp;
	double end_timestamp;
};

struct style_element_internal
{
	struct style_element;
	struct tweener* keyframe_tweener;
	double variation;

	struct particle* particles;
	size_t particles_allocated;
	size_t particles_used;
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

	make_shader();
	tweener_init();
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

	style_element->particles = NULL;
	style_element->particles_allocated = 0;
	style_element->particles_used = 0;

	return (struct style_element*)style_element;
}

void style_element_enter_loop(struct style_element* const style_element, double looping_offset)
{
	struct style_element_internal* const internal = (struct style_element_internal* const)style_element;
	tweener_enter_loop(internal->keyframe_tweener, looping_offset);
}

static void style_element_update_work(struct style_element_internal* style_element)
{
	for (size_t i = 0; i < style_element->particles_used; i++)
		if (style_element->particles[i].end_timestamp <= current_timestamp)
			style_element->particles[i--] = style_element->particles[--style_element->particles_used];

	double* keypoint = style_element->keyframe_tweener->current;

	style_element->current.x = keypoint[0];
	style_element->current.y = keypoint[1];
	style_element->current.sx = keypoint[2];
	style_element->current.sy = keypoint[3];
	style_element->current.theta = keypoint[4];
}

struct work_queue* style_element_update()
{
	struct work_queue* work_queue = tweener_update();

	for (size_t i = 0; i < used; i++)
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

	style_element_apply_material(style_element, NULL);

	al_use_transform(&buffer);

	glDisable(GL_STENCIL_TEST);
}

void style_element_apply_material(
	const struct style_element* const style_element,
	struct material* material)
{
	material_set_shader(material);
}

void style_element_particle_new(struct style_element* const style_element, void (*draw)(double,size_t), double end_timestamp, size_t seed)
{
	struct style_element_internal* const internal = (struct style_element_internal* const)style_element;

	if (internal->particles_allocated <= internal->particles_used)
	{
		const size_t new_cnt = 2 * internal->particles_allocated+1;

		struct particle* memsafe_hande = realloc(internal->particles, new_cnt * sizeof(struct style_element_internal));

		if (!memsafe_hande)
			return;

		internal->particles = memsafe_hande;
		internal->particles_allocated = new_cnt;
	}

	struct particle* particle = internal->particles + internal->particles_used++;

	particle->draw = draw;
	particle->end_timestamp = current_timestamp+end_timestamp;
	particle->start_timestamp = current_timestamp;
}

void style_element_draw_particles(const struct style_element* const style_element)
{
	struct style_element_internal* const internal = (struct style_element_internal* const)style_element;

	for (size_t i = 0; i < internal->particles_used; i++)
	{
		const struct particle* const particle = internal->particles + i;

		particle->draw(current_timestamp - particle->start_timestamp, particle->seed);
	}

}
