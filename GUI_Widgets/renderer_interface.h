// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once
#include <allegro5/allegro_color.h>

// Simple transparent keyframe object meant to represent the all the data needed to make a transform at a given time.
#define FOR_KEYFRAME_MEMBERS_TIMELESS(DO)\
    DO(x, 1) \
    DO(y, 2) \
    DO(sx, 3) \
    DO(sy, 4) \
    DO(theta, 5) \
	DO(camera, 6) \
	DO(dx, 7) \
	DO(dy, 8)

#define FOR_KEYFRAME_MEMBERS(DO) \
    DO(timestamp, 0) \
	FOR_KEYFRAME_MEMBERS_TIMELESS(DO)

#define KEYFRAME_MEMBER_CNT 8

#define _KEYFRAME_MEMBER_DECL(X,...) double X;

struct keyframe
{
	FOR_KEYFRAME_MEMBERS(_KEYFRAME_MEMBER_DECL)
};

void keyframe_default(struct keyframe* const);
void keyframe_build_transform(const struct keyframe* const, ALLEGRO_TRANSFORM* const);

// An obficated struct for managing all the style information in one place.
//	It's maing job is to manage a queue of keyframes to output the current keyframe on each frame.
//	Has other jobs like manage some effects and to be added drag control.
struct render_interface
{
	struct keyframe current;
	double half_width, half_height;
};

struct render_interface* render_interface_new(size_t);

// Keyframe methods
void render_interface_set(struct render_interface* const, struct keyframe* const);
void render_interface_interupt(struct render_interface* const);
void render_interface_push_keyframe(struct render_interface* const, struct keyframe*);
void render_interface_copy_destination(struct render_interface* const, struct keyframe*);
void render_interface_enter_loop(struct render_interface* const, double);
void render_interface_callback(struct sytle_element* const, void (*)(void*), void*);

// Effect 
enum STYLE_EFFECT_ID
{
	STYLE_EFFECT_ID_SATURATE,
	STYLE_EFFECT_ID_BURN,
	STYLE_EFFECT_ID_FREEZE
};

// Effect methods
void render_interface_effect(struct render_interface* const, enum STYLE_EFFECT_ID, double);
