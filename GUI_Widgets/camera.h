// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once

#include <allegro5/allegro.h>
#include "renderer_interface.h"

ALLEGRO_TRANSFORM camera_transform;

void camera_compose_transform(ALLEGRO_TRANSFORM* const, const double);
void camera_set_keyframe(const struct keyframe* const);
void camera_push_keyframe(const struct keyframe* const);
void camera_interupt();