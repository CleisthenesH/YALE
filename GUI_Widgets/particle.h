// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once

// A Particle is a simple object with three self-explanitory methods.
// They are meant to be light wight and only for adding some punch to animation.

struct particle_jumptable
{
	void (*draw)(void*, double);
	void (*update)(void*, double);
	void (*gc)(void*);
};

struct particle_bin* particle_bin_new(size_t);

void particle_bin_append(struct particle_bin*, const struct particle_jumptable* const, void*, double);
void particle_bin_draw(struct particle_bin*);
