// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once
#include <allegro5/allegro_color.h>

// Simple transparent keyframe object meant to represent the all the data needed to make a transform at a given time
#define FOR_KEYFRAME_MEMBERS(DO) \
    DO(timestamp) \
    DO(x) \
    DO(y) \
    DO(sx) \
    DO(sy) \
    DO(theta) \

struct keyframe
{
	double timestamp, x, y, sx, sy, theta;
};

void keyframe_default(struct keyframe* const);
void keyframe_build_transform(const struct keyframe* const, ALLEGRO_TRANSFORM* const);

// Effect 
enum STYLE_EFFECT_ID
{
	STYLE_EFFECT_ID_SATURATE,
	STYLE_EFFECT_ID_BURN,
	STYLE_EFFECT_ID_FREEZE
};

// An obficated struct for managing all the style information in one place.
//	It's maing job is to manage a queue of keyframes to output the current keyframe on each frame.
//	Has other jobs like manage some effects and to be added drag control.
struct render_interface
{
	struct keyframe current;
	double width, height;
};

struct render_interface* render_interface_new(size_t);

// Keyframe methods
void render_interface_set(struct render_interface* const, struct keyframe* const);
void render_interface_interupt(struct render_interface* const);
void render_interface_push_keyframe(struct render_interface* const, struct keyframe*);
void render_interface_align_tweener(struct render_interface* const);
void render_interface_copy_destination(struct render_interface* const, struct keyframe*);
void render_interface_enter_loop(struct render_interface* const, double);
void render_interface_callback(struct sytle_element* const, void (*)(void*), void*);

// Effect methods
void render_interface_effect(struct render_interface* const, enum STYLE_EFFECT_ID, double);
