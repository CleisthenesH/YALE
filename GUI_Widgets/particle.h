// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once

struct particle_bin
{
	struct particle* particles;
	size_t particles_allocated;
	size_t particles_used;
};

void particle_bin_append(struct particle_bin* bin, void (*draw)(double, size_t), double end_timestamp, size_t seed);
void particle_bin_draw(struct particle_bin*);
void particle_bin_update(struct particle_bin*);