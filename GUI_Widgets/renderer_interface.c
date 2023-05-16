// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "renderer_interface.h"
#include "thread_pool.h"
#include "tweener.h"
#include "material.h"

#include <stdio.h>
#include <math.h>
#include <allegro5/allegro_opengl.h>

extern double current_timestamp;

static ALLEGRO_SHADER* shader;

struct render_interface_internal
{
	struct render_interface;
	struct tweener* keyframe_tweener;
	double variation;
};

static struct render_interface_internal* list;
static size_t allocated;
static size_t used;

#ifdef _CHECK_KEYFRAME_DEBUG
static void assert_current_keyframe_nan(struct render_interface* render)
{
	if (render)
		if (isnan(render->current.x) ||
			isnan(render->current.y) ||
			isnan(render->current.sx) ||
			isnan(render->current.sy) ||
			isnan(render->current.theta))
			printf("ERROR: %p's Current frame has nan component\n", render);
}

#define CHECK_NON_NAN_CURRENT_FRAME(render) assert_current_keyframe_nan((render));
#else
#define CHECK_NON_NAN_CURRENT_FRAME(render);
#endif

static inline int make_shader()
{
	shader = al_create_shader(ALLEGRO_SHADER_GLSL);

	if (!al_attach_shader_source_file(shader, ALLEGRO_VERTEX_SHADER, "shaders/main_renderer.vert"))
	{
		fprintf(stderr, "Failed to attach main renderer vertex shader.\n%s\n", al_get_shader_log(shader));
		return 0;
	}

	if (!al_attach_shader_source_file(shader, ALLEGRO_PIXEL_SHADER, "shaders/main_renderer.frag"))
	{
		fprintf(stderr, "Failed to attach main renderer pixel shader.\n%s\n", al_get_shader_log(shader));
		return 0;
	}

	if (!al_build_shader(shader))
	{
		fprintf(stderr, "Failed to build main renderer shader.\n%s\n", al_get_shader_log(shader));
		return 0;
	}

	ALLEGRO_DISPLAY* const display = al_get_current_display();
	const float dimensions[2] = { al_get_display_width(display),al_get_display_height(display) };

	al_use_shader(shader);
	al_set_shader_float_vector("display_dimensions", 2, dimensions, 1);
	al_use_shader(NULL);

	return 1;
}

void render_interface_init()
{
	used = 0;
	allocated = 100;
	list = malloc(allocated * sizeof(struct render_interface_internal));

	make_shader();
}

struct render_interface* render_interface_new(size_t hint)
{
	if (allocated <= used)
	{
		const size_t new_cnt = 2 * allocated;

		struct render_interface_internal* memsafe_hande = realloc(list, new_cnt * sizeof(struct render_interface_internal));

		if (!memsafe_hande)
			return NULL;

		list = memsafe_hande;
		allocated = new_cnt;
	}

	struct render_interface_internal* render_interface = list + used++;

	if (hint < 1)
		hint = 1;

	render_interface->keyframe_tweener = tweener_new(5, hint);
	render_interface->variation = fmod(current_timestamp, 100);
	render_interface->half_width = 0;
	render_interface->half_height = 0;

	return (struct render_interface*)render_interface;
}

void render_interface_enter_loop(struct render_interface* const render_interface, double looping_offset)
{
	struct render_interface_internal* const internal = (struct render_interface_internal* const)render_interface;
	tweener_enter_loop(internal->keyframe_tweener, looping_offset);
}

static void render_interface_update_work(struct render_interface_internal* render_interface)
{
	double* keypoint = render_interface->keyframe_tweener->current;

	render_interface->current.x = keypoint[0];
	render_interface->current.y = keypoint[1];
	render_interface->current.sx = keypoint[2];
	render_interface->current.sy = keypoint[3];
	render_interface->current.theta = keypoint[4];

	CHECK_NON_NAN_CURRENT_FRAME((struct render_interface*) render_interface)

}

struct work_queue* render_interface_update()
{
	struct work_queue* work_queue = work_queue_create();

	for(struct render_interface_internal* p = list;
		p != (used ? list + used : list);
		p++)
		work_queue_push(work_queue, render_interface_update_work, p);

	return work_queue;
}

void render_interface_set(struct render_interface* const render_interface, struct keyframe* const set)
{
	struct render_interface_internal* const internal = (struct render_interface_internal* const)render_interface;
	struct tweener* const tweener = internal->keyframe_tweener;

	tweener_set(tweener, (double[]) { set->x, set->y, set->sx, set->sy, set->theta });

	memcpy(&render_interface->current, set, sizeof(struct keyframe));  // maybe can be optimized out

	CHECK_NON_NAN_CURRENT_FRAME(render_interface)
}

void render_interface_callback(struct sytle_element* const render_interface, void (*funct)(void*), void* data)
{
	struct render_interface_internal* const internal = (struct render_interface_internal* const)render_interface;
	struct tweener* const tweener = internal->keyframe_tweener;

	tweener_set_callback(tweener, funct, data);
}

void render_interface_push_keyframe(struct render_interface* const render_interface, struct keyframe* frame)
{
	struct render_interface_internal* const internal = (struct render_interface_internal* const)render_interface;
	double* new_point = tweener_new_point(internal->keyframe_tweener);

	new_point[0] = frame->timestamp;
	new_point[1] = frame->x;
	new_point[2] = frame->y;
	new_point[3] = frame->sx;
	new_point[4] = frame->sy;
	new_point[5] = frame->theta;
}

void render_interface_copy_destination(struct render_interface* const render_interface, struct keyframe* keyframe)
{
	struct render_interface_internal* const internal = (struct render_interface_internal* const)render_interface;

	double* keypoints = tweener_destination(internal->keyframe_tweener);

	keyframe->timestamp = keypoints[0];
	keyframe->x = keypoints[1];
	keyframe->y = keypoints[2];
	keyframe->sx = keypoints[3];
	keyframe->sy = keypoints[4];
	keyframe->theta = keypoints[5];
}

// Align the tweener's current with the render_interface's current
void render_interface_align_tweener(struct render_interface* const render_interface)
{
	render_interface_set(render_interface, &render_interface->current);
}

void render_interface_interupt(struct render_interface* const render_interface)
{
	struct render_interface_internal* const internal = (struct render_interface_internal* const)render_interface;

	tweener_interupt(internal->keyframe_tweener);

	internal->current.x = internal->keyframe_tweener->current[0];
	internal->current.y = internal->keyframe_tweener->current[1];
	internal->current.sx = internal->keyframe_tweener->current[2];
	internal->current.sy = internal->keyframe_tweener->current[3];
	internal->current.theta = internal->keyframe_tweener->current[4];

	CHECK_NON_NAN_CURRENT_FRAME(render_interface)
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

void render_interface_global_predraw()
{
	al_use_shader(shader);
	glDisable(GL_STENCIL_TEST);
	al_set_shader_float("current_timestamp", current_timestamp);
}

void render_interface_predraw(const struct render_interface* const render_interface)
{
	struct render_interface_internal* const internal = (struct render_interface_internal* const)render_interface;

	//al_set_shader_float("saturate", render_interface->current.saturate);
	al_set_shader_float("variation", internal->variation);

	// You don't actually have to send this everytime, should track a count of materials that need it.
	if (render_interface->half_width != 0 && render_interface->half_height != 0)
	{
		const float dimensions[2] = { 1.0/ render_interface->half_width, 1.0 / render_interface->half_height };
		al_set_shader_float_vector("object_scale", 2, dimensions, 1);
	}

	ALLEGRO_TRANSFORM buffer;
	keyframe_build_transform(&render_interface->current, &buffer);

	material_apply(NULL);

	al_use_transform(&buffer);

	glDisable(GL_STENCIL_TEST);
}
