// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "renderer_interface.h"
#include "thread_pool.h"
#include "tweener.h"
#include "material.h"
#include "camera.h"

#include <stdio.h>
#include <math.h>
#include <allegro5/allegro_opengl.h>

extern void camera_init();
extern void camera_global_predraw();

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
	camera_init();
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

	render_interface->keyframe_tweener = tweener_new(KEYFRAME_MEMBER_CNT, hint);
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

#define _KEYFRAME_COPY_KP_CU(X,IDX,...) render_interface->current.## X = keypoint[IDX-1];
	FOR_KEYFRAME_MEMBERS_TIMELESS(_KEYFRAME_COPY_KP_CU)

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

	tweener_set(tweener, (double[]) { set->x, set->y, set->sx, set->sy, set->theta , set->camera, set->dx, set->dy});

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

#define _KEYFRAME_COPY_FR_NP(X,IDX,...) new_point[IDX] = frame->## X ;
	FOR_KEYFRAME_MEMBERS(_KEYFRAME_COPY_FR_NP)
}

void render_interface_copy_destination(struct render_interface* const render_interface, struct keyframe* keyframe)
{
	struct render_interface_internal* const internal = (struct render_interface_internal* const)render_interface;

	double* keypoints = tweener_destination(internal->keyframe_tweener);

#define _KEYFRAME_COPY_NP_FR(X,IDX,...) keyframe->## X = keypoints[IDX];
	FOR_KEYFRAME_MEMBERS(_KEYFRAME_COPY_NP_FR)
}

void render_interface_interupt(struct render_interface* const render_interface)
{
	struct render_interface_internal* const internal = (struct render_interface_internal* const)render_interface;

	tweener_interupt(internal->keyframe_tweener);

#define _KEYFRAME_COPY_CR_TW(X,IDX,...) render_interface->current.## X = internal->keyframe_tweener->current[IDX-1];
	FOR_KEYFRAME_MEMBERS_TIMELESS(_KEYFRAME_COPY_CR_TW)

	CHECK_NON_NAN_CURRENT_FRAME(render_interface)
}

void keyframe_build_transform(const struct keyframe* const keyframe, ALLEGRO_TRANSFORM* const trans)
{
	al_build_transform(trans,
		keyframe->x, keyframe->y,
		keyframe->sx, keyframe->sy,
		keyframe->theta);

	if (keyframe->camera >= 0.0)
		camera_compose_transform(trans, keyframe->camera);

	al_translate_transform(trans, keyframe->dx, keyframe->dy);
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
	camera_global_predraw();
}

void render_interface_use_transform(const struct render_interface* const render_interface)
{
	//TODO probably make a global for some perfonacne.
	//		Thread safty?

	ALLEGRO_TRANSFORM buffer;
	keyframe_build_transform(&render_interface->current, &buffer);
	al_use_transform(&buffer);
}

void render_interface_predraw(const struct render_interface* const render_interface)
{
	struct render_interface_internal* const internal = (struct render_interface_internal* const)render_interface;

	//al_set_shader_float("saturate", render_interface->current.saturate);
	al_set_shader_float("variation", internal->variation);

	// You don't actually have to send this everytime, should track a count of materials that need it.
	if (render_interface->half_width != 0 && render_interface->half_height != 0)
	{
		const float dimensions[2] = { 1.0 / render_interface->half_width, 1.0 / render_interface->half_height };
		al_set_shader_float_vector("object_scale", 2, dimensions, 1);
	}

	render_interface_use_transform(render_interface);
	material_apply(NULL);

	glDisable(GL_STENCIL_TEST);
}
