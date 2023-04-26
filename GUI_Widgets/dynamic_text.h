// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once

#include "particle.h"
#include "resource_manager.h"

enum dynamic_text_animation
{
	DYNAMIC_TEXT_WORM
};

void dynamic_text_new(struct particle_bin* bin, 
	const char* text, 
	double x, double y, double scale,
	enum font_id font_id, 
	enum dynamic_text_animation animation_id, 
	double duration);
