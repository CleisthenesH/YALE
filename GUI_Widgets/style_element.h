// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#pragma once
#include <allegro5/allegro_color.h>

enum effect_flags
{
	EFFECT_NULL = 0,
	EFFECT_BLEND = 1,
	EFFECT_SATURATE = 2,
	EFFECT_FOILED = 4,
};

enum foil_effect
{
	FOIL_NULL,
	FOIL_PLAIN,
	FOIL_CIRCLE,
	FOIL_CRESCENT,
};

struct keyframe
{
	double timestamp;

	double x, y, sx, sy, t;
	double saturate;
	ALLEGRO_COLOR blender_color;
};

struct style_element
{
	enum effect_flags effect_flags;
	struct keyframe current;
	enum foil_effect stencil_effects[4];
	int blender[3];
};

struct style_element* style_element_new(size_t);

void style_element_set(struct style_element* const, struct keyframe* const);
void style_element_interupt();
struct keyframe* style_element_new_frame(struct style_element* const);

void style_element_build_transform(struct style_element* const,ALLEGRO_TRANSFORM*); // include with picker

// should be removed from here
void style_element_setup();
void style_element_predraw(const struct style_element* const);
void style_element_prefoiling(const struct style_element* const);
void style_element_effect(const struct style_element* const, int);
