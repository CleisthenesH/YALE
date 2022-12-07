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
	EFFECT_FOIL = 4,		// What if I only want some parts foiled? Make it interact with a stencil flag?
	EFFECT_FLAME = 8,
};

struct keyframe
{
	double timestamp;
	enum effect_flags effect_flags;

	double x, y, sx, sy, t;
	double saturate;
};

struct style_element;

struct style_element* style_element_new(size_t);

void style_element_set(struct style_element*, struct keyframe*);
void style_element_interupt();
struct keyframe* style_element_new_frame(struct style_element*);

struct keyframe* styel_element_current_frame(struct style_element*);
void style_element_build_transform(ALLEGRO_TRANSFORM*);
void style_element_apply(struct style_element*);

