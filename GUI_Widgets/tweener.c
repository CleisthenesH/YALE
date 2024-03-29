// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "thread_pool.h"
#include "tweener.h"

#include <limits.h>

extern double current_timestamp;

static struct tweener* tweeners_list;
static size_t tweeners_allocated;
static size_t tweeners_used;

#ifdef _TWEENER_DEBUG
#include <stdio.h>
static void _check_nan(double* ptr, size_t cnt)
{
	bool flag = false;

	for (size_t i = 0; i < cnt; i++)
	{
		if (isnan(ptr[i]))
			flag = true;

		break;
	}
	
	if (flag)
		printf("ERROR: tweener nan\n");		
}

static void _print_keyframes(struct tweener* tweener)
{
	printf("tweener %p\t, channels: %ld, used %ld\n", tweener, tweener->channels, tweener->used);

	for (size_t i = 0; i < tweener->used; i++)
	{
		printf("\t time: %lf\t channels: ", tweener->keypoints[i * (tweener->channels + 1)]);
		for (size_t j = 1; j <= tweener->channels; j++)
			printf("%lf,", tweener->keypoints[i * (tweener->channels + 1) + j]);

		printf("\n");
	}
}

#define CHECK_TWEENER_NAN(tweener) _check_nan((tweener)->current,(tweener)->channels);
#define CHECK_KEYPOINT_NAN(ptr,cnt) _check_nan((ptr),(cnt));
#define PRINT_KEYFRAMES(tweener) _print_keyframes((tweener));

#else
#define CHECK_TWEENER_NAN(tweener);
#define CHECK_KEYPOINT_NAN(ptr,cnt);
#define PRINT_KEYFRAMES(tweener);
#endif

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
	tweener->looping_time = -1;
	tweener->funct = NULL;
	tweener->data = NULL;
	tweener->looping_idx = 0;

	return (struct tweener*)tweener;
}

void tweener_del(struct tweener* tweener)
{
	free(tweener->keypoints);
	free(tweener->current);

	//TODO: pop the tweener from tweener_list and free the struct
}

static inline void tweener_blend_nonlooping(struct tweener* tweener)
{
	// Clean old frames;
	size_t first_future_frame = 0;

	while (tweener->keypoints[first_future_frame * (tweener->channels + 1)] < current_timestamp &&
		first_future_frame < tweener->used)
		first_future_frame++;

	// Clean frames with two similar timestamps (numeric stability)
	while ((0.01 > tweener->keypoints[(first_future_frame +1)* (tweener->channels + 1)] - tweener->keypoints[first_future_frame * (tweener->channels + 1)]) &&
		first_future_frame +1 < tweener->used)
		first_future_frame++;

	if (first_future_frame > 1)
	{
		const size_t step = first_future_frame - 1;

		for (size_t i = step; i < tweener->used; i++)
			for (size_t j = 0; j <= tweener->channels; j++)
				tweener->keypoints[(i - step) * (tweener->channels + 1) + j] = tweener->keypoints[i * (tweener->channels + 1) + j];

		tweener->used -= step;

		PRINT_KEYFRAMES(tweener)

		if (tweener->used == 1)
		{
			memcpy(tweener->current, tweener->keypoints + 1, tweener->channels * sizeof(double));

			CHECK_TWEENER_NAN(tweener);

			if (tweener->funct)
				tweener->funct(tweener->data);

			return;
		}
	}

	PRINT_KEYFRAMES(tweener)
	
	const double denominator = (tweener->keypoints[tweener->channels + 1] - tweener->keypoints[0]);
	const double blend = (current_timestamp - tweener->keypoints[0]) / denominator;

	for (size_t i = 0; i < tweener->channels; i++)
		tweener->current[i] = blend * tweener->keypoints[i + 2 + tweener->channels] + (1 - blend) * tweener->keypoints[i + 1];

	CHECK_TWEENER_NAN(tweener);
}

static inline void tweener_blend_looping(struct tweener* tweener)
{
	while (tweener->keypoints[tweener->looping_idx * (tweener->channels + 1)] <= current_timestamp)
	{
		const size_t back_idx = (tweener->looping_idx >= 1) ? 
			tweener->looping_idx - 1 : tweener->used - 1;

		tweener->keypoints[back_idx * (tweener->channels + 1)] += tweener->looping_time;

		tweener->looping_idx = (tweener->looping_idx != SIZE_MAX) ? 
			(tweener->looping_idx + 1) % tweener->used : 0;
	}

	const size_t end_idx = tweener->looping_idx * (tweener->channels + 1);
	const size_t start_idx = (tweener->channels + 1) * ((tweener->looping_idx >= 1) ? 
		(tweener->looping_idx - 1) : tweener->used - 1);
	
	const double blend = (current_timestamp - tweener->keypoints[start_idx]) / 
		(tweener->keypoints[end_idx] - tweener->keypoints[start_idx]);

	for (size_t i = 0; i < tweener->channels; i++)
		tweener->current[i] = blend * tweener->keypoints[i + end_idx + 1] + 
			(1 - blend) * tweener->keypoints[i + start_idx + 1];

	CHECK_TWEENER_NAN(tweener);
}

static inline void tweener_blend_keypoints(struct tweener* const tweener)
{
	if (tweener->used > 1)
		if (tweener->looping_time > 0)
			tweener_blend_looping(tweener);
		else
			tweener_blend_nonlooping(tweener);

	CHECK_TWEENER_NAN(tweener);
}

struct work_queue* tweener_update()
{
	struct work_queue* work_queue = work_queue_create();

	for (struct tweener* p = tweeners_list;
		p != (tweeners_used ? tweeners_list + tweeners_used : tweeners_list);
		p++)
		if (p->used > 1)
			if(p->looping_time > 0)
				work_queue_push(work_queue, tweener_blend_looping, p);
			else
				work_queue_push(work_queue, tweener_blend_nonlooping, p);

	return work_queue;
}

void tweener_set(struct tweener* const tweener, double* keypoint)
{
	tweener->used = 1;

	memcpy(tweener->current, keypoint, tweener->channels * sizeof(double));
	memcpy(tweener->keypoints + 1, keypoint, tweener->channels * sizeof(double));

	CHECK_TWEENER_NAN(tweener);
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
	if (tweener->used < 2)
		return;

	// Can optimize using a division to get loops
	size_t idx = 0;
	size_t loops = 0;
	const double loop_time = tweener->keypoints[(tweener->used - 1) * (tweener->channels + 1)] - tweener->keypoints[0] + loop_offset;

	while (tweener->keypoints[idx * (tweener->channels + 1)] <= current_timestamp - ((double)loops) * loop_time)
		if (++idx == tweener->used)
			loops++, idx = 0;

	if (loops)
		for (size_t i = 0; i < tweener->used; i++)
			tweener->keypoints[i * (tweener->channels + 1)] += ((double)loops) * loop_time;

	tweener->looping_idx = idx;
	tweener->looping_time = loop_time;
}

void tweener_interupt(struct tweener* const tweener)
{
	if (tweener->used > 1)
		tweener_blend_keypoints(tweener);

	tweener->looping_time = -1;

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
		double* new_point = tweener_new_point(tweener);

		funct(timestamp, new_point, udata);
	}
}
