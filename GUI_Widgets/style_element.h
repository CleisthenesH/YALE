// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#pragma once
#include <allegro5/allegro_color.h>

// Simple transparent keyframe object meant to represent the all the data needed to make a transform at a given time
struct keyframe
{
	double timestamp,x, y, sx, sy, theta;
};

void keyframe_default(struct keyframe* const);
void keyframe_build_transform(const struct keyframe* const, ALLEGRO_TRANSFORM* const);

// An obficated element struct.
//	The obfiscation is because the internal variables of a effects can change wildly but shaders are exact.
//	So I want to mantain the effect state where the user can't mess with it.
//	Might add and update element method to them at later date, might not.
struct effect_element;

enum SELECTION_ID
{
	SELECTION_ID_FULL,
	SELECTION_ID_COLOR_BAND,
};

enum EFFECT_ID
{
	EFFECT_ID_NULL,
	EFFECT_ID_PLAIN_FOIL,
	EFFECT_ID_RADIAL_RGB,
/*	EFFECT_ID_SHEEN,
	EFFECT_ID_COLOR_SELECT,
	EFFECT_ID_TEST,
*/
};

struct effect_element* effect_element_new(enum EFFECT_ID, enum SELECTION_ID);

void effect_element_point(struct effect_element* const, double, double);
void effect_element_color(struct effect_element* const, ALLEGRO_COLOR);
void effect_element_cutoff(struct effect_element* const, double);

void effect_element_selection_color(struct effect_element* const, ALLEGRO_COLOR);
void effect_element_selection_cutoff(struct effect_element* const, double);

// An obficated struct for managing all the style information in one place.
//	It's maing job is to manage a queue of keyframes to output the current keyframe on each frame.
//	Has other jobs like manage some effects and to be added drag control.
struct style_element
{
	struct keyframe current;
	double width, height;
};

struct style_element* style_element_new(size_t);

// Want to simplify adding a new keyframe and the interface in general
// it's not too much to ask the widget maker to track timestamps
// especially if we let them check the current and destination keyframe
void style_element_set(struct style_element* const, struct keyframe* const);
void style_element_interupt(struct style_element* const);
void style_element_push_keyframe(struct style_element* const, struct keyframe*);
void style_element_copy_destination(struct style_element* const, struct keyframe*);
void style_element_enter_loop(struct style_element* const, double);

void style_element_effect(const struct style_element* const, struct effect_element*);
