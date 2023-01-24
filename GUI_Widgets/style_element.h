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

// An obficated struct for a rendering material.
//	The obfiscation is because the internal variables of a material can change wildly but the shaders requirements are exact.
//	So I want to mantain the material's state where the user can't mess with it.
//	Might add and update element method to them at later date, might not.
struct material;

enum EFFECT_ID
{
	EFFECT_ID_NULL,
	EFFECT_ID_PLAIN_FOIL,
	EFFECT_ID_RADIAL_RGB,
	/*
		EFFECT_ID_SHEEN,
	*/
};

enum SELECTION_ID
{
	SELECTION_ID_FULL,
	SELECTION_ID_COLOR_BAND,
};

struct material* material_new(enum EFFECT_ID, enum SELECTION_ID);

void material_effect_point(struct material* const, double, double);
void material_effect_color(struct material* const, ALLEGRO_COLOR);
void material_effect_cutoff(struct material* const, double);

void material_selection_color(struct material* const, ALLEGRO_COLOR);
void material_selection_cutoff(struct material* const, double);

// An obficated struct for managing all the style information in one place.
//	It's maing job is to manage a queue of keyframes to output the current keyframe on each frame.
//	Has other jobs like manage some effects and to be added drag control.
struct style_element
{
	struct keyframe current;
	double width, height;
};

struct style_element* style_element_new(size_t);

// Keyframe methods
void style_element_set(struct style_element* const, struct keyframe* const);
void style_element_interupt(struct style_element* const);
void style_element_push_keyframe(struct style_element* const, struct keyframe*);
void style_element_align_tweener(struct style_element* const);
void style_element_copy_destination(struct style_element* const, struct keyframe*);
void style_element_enter_loop(struct style_element* const, double);
void style_element_callback(struct sytle_element* const, void (*)(void*), void*);

// Material methods
void style_element_apply_material(const struct style_element* const, struct material*);

// An obficated struct for a rendering a particle.
//  Particles have a limited life span and are likely to go stale so they don't exist seperate to the style element.
//	So they are being implemented as a set and forget way atm, may comeback and change later.
//	May need to add "layers" to particles in the style_element though?

enum PARTICLE_ID
{
	PARTICLE_ID_CIRCLE,
};

void style_element_particle_new(struct style_element* const, enum PARTICLE_ID, void*);
void style_element_draw_particles(const struct style_element* const);
