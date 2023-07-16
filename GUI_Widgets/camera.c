// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "camera.h"
#include "tweener.h"

static struct tweener* camera_tweener;

void camera_init()
{
	camera_tweener = tweener_new(5, 0);

	tweener_set(camera_tweener, (double[]) { 0, 0, 1, 1, 0.1 });

	if (0)
	{

		struct keyframe keyframe;
		keyframe_default(&keyframe);
		keyframe.timestamp += 5;
		keyframe.theta += 1;

		camera_push_keyframe(&keyframe);
	}
}

void camera_global_predraw()
{
	al_build_transform(&camera_transform,
		camera_tweener->current[0],
		camera_tweener->current[1],
		camera_tweener->current[2],
		camera_tweener->current[3],
		camera_tweener->current[4]
	);
}

void camera_push_keyframe(const struct keyframe* const frame)
{
	double* new_point = tweener_new_point(camera_tweener);

	new_point[0] = frame->timestamp;
	new_point[1] = frame->x;
	new_point[2] = frame->y;
	new_point[3] = frame->sx;
	new_point[4] = frame->sy;
	new_point[5] = frame->theta;
}

void camera_set_keyframe(const struct keyframe* const set)
{
	tweener_set(camera_tweener, (double[]) { set->x, set->y, set->sx, set->sy, set->theta });
}

void camera_interupt()
{
	tweener_interupt(camera_tweener);
}

void camera_compose_transform(ALLEGRO_TRANSFORM* const trans, const double blend)
{
	ALLEGRO_TRANSFORM buffer;

	const double blend_x = camera_tweener->current[0] * blend;
	const double blend_y = camera_tweener->current[1] * blend;
	const double blend_sx = camera_tweener->current[2] * blend +(1-blend);
	const double blend_sy = camera_tweener->current[3] * blend+(1-blend);
	const double blend_theta = camera_tweener->current[4] * blend;

	al_build_transform(&buffer,
		blend_x, blend_y,
		blend_sx, blend_sy,
		blend_theta);

	al_compose_transform(trans, &buffer);
}

void camera_copy_destination(struct keyframe* const keyframe)
{
	double* coords =  tweener_destination(camera_tweener);

	keyframe->timestamp = coords[0];
	keyframe->x = coords[1];
	keyframe->y = coords[2];
	keyframe->sx = coords[3];
	keyframe->sy = coords[4];
	keyframe->theta = coords[5];
}