// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// The tweener struct takes timestamped keypoints and blends them based on the current timestamp.
//	The main use case is to handle the varitables for the al_build_transform function (position, scale, angle).
//  Other use cases include a color or threshold for an effect.
// 
//	The structs memory is quite raw so it is only avavlible to widget writers.
//	The user should be using the render_interface.
// 
//	TODO: Include more complex tweening options like keypoint weight and tangent.
#pragma once

struct tweener
{
	size_t channels; //	The zeroth channel is the timestamp
	double* current;

	size_t used, allocated;
	double* keypoints; // could optimize better with flexable array member?

	// Looping data
	size_t looping_idx;
	double looping_time;

	// Callback when the path ends 
	void (*funct)(void*);
	void* data;
};

struct tweener* tweener_new(size_t channels, size_t hint);
void tweener_del(struct tweener* tweener);

void tweener_set(struct tweener* const tweener, double* keypoint);
double* tweener_new_point(struct tweener* tweener);
void tweener_enter_loop(struct tweener* tweener, double loop_offset);
void tweener_interupt(struct tweener* const tweener);
double* tweener_destination(struct tweener* const tweener);

void tweener_set_callback(struct tweener* const tweener, void (*funct)(void*), void* data);

// Not tested
void tweener_plot(struct tweener* const tweener,
	void (*funct)(double timestamp, double* output, void* udata), void* udata,
	double start, double end, size_t steps);