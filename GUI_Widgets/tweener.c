// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "thread_pool.h"
#include "tweener.h"

extern double current_timestamp;

static struct tweener* tweeners_list;
static size_t tweeners_allocated;
static size_t tweeners_used;

void tweener_init()
{
	tweeners_used = 0;
	tweeners_allocated = 100;
	tweeners_list = malloc(tweeners_allocated * sizeof(struct tweener));
}

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
	// Pretty sure it exits the loop one to early if idx = 0.
	// That's why there is +loop_time in the blend	
	// this will need to be fixed if we wish to store idx between calls

	size_t loops = 0;

	// can delay calculating this
	double loop_time = tweener->keypoints[(tweener->used - 1) * (tweener->channels + 1)] - tweener->keypoints[0] + tweener->looping_offset; 
	
	// maybe keep this index between calls	
	// fint the first future frame
	size_t idx = 0; 

	while (tweener->keypoints[idx * (tweener->channels + 1)] <= current_timestamp - ((double)loops) * loop_time)
		if (++idx == tweener->used)
			loops++, idx = 0;

	// adjust timestamps to proper range
	if(loops)
		for (size_t i = 0; i < tweener->used; i++)
			tweener->keypoints[i * (tweener->channels + 1)] += ((double)loops) * loop_time;

	//
	const size_t end_idx = idx * (tweener->channels + 1);
	const size_t start_idx = (tweener->channels + 1) * ((idx != 0) ? (idx - 1) : tweener->used - 1);

	double tmp_blend;

	if (idx != 0)
		tmp_blend = (current_timestamp - tweener->keypoints[start_idx]) / (tweener->keypoints[end_idx] - tweener->keypoints[start_idx]);
	else
		tmp_blend = (current_timestamp - tweener->keypoints[start_idx]+loop_time) / tweener->looping_offset;

	const double blend = tmp_blend;

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

void tweener_set(struct tweener* const tweener, double* keypoint)
{
	tweener->used = 1;

	memcpy(tweener->current, keypoint, tweener->channels * sizeof(double));
	memcpy(tweener->keypoints + 1, keypoint, tweener->channels * sizeof(double));
}

double* tweener_new_point(struct tweener* tweener)
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

void tweener_enter_loop(struct tweener* tweener, double loop_offset)
{
	tweener->looping = true;
	tweener->looping_offset = loop_offset;
}

void tweener_interupt(struct tweener* const tweener)
{
	if (tweener->used > 1)
		tweener_blend_keypoints(tweener);

	tweener->looping = false;

	tweener_set(tweener, tweener->current);
}

double* tweener_destination(struct tweener* const tweener)
{
	return tweener->keypoints + (tweener->used - 1) * (tweener->channels + 1);
}

void tweener_set_callback(struct tweener* const tweener, void (*funct)(void*), void* data)
{
	tweener->funct = funct;
	tweener->data = data;
}

// Not tested
void tweener_plot(struct tweener* const tweener,
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
