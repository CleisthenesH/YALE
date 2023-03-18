// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.


#include "particle.h"
#include "style_element.h"
extern double current_timestamp;

struct particle
{
	void (*draw)(double, size_t);
	size_t seed;
	double start_timestamp;
	double end_timestamp;
};

void particle_bin_append(struct particle_bin* bin, void (*draw)(double, size_t), double end_timestamp, size_t seed)
{
	if (bin->particles_allocated <= bin->particles_used)
	{
		const size_t new_cnt = 2 * bin->particles_allocated + 1;

		struct particle* memsafe_hande = realloc(bin->particles, new_cnt * sizeof(struct particle));

		if (!memsafe_hande)
			return;

		bin->particles = memsafe_hande;
		bin->particles_allocated = new_cnt;
	}

	struct particle* particle = bin->particles + bin->particles_used++;

	particle->draw = draw;
	particle->end_timestamp = current_timestamp + end_timestamp;
	particle->start_timestamp = current_timestamp;
}

void particle_bin_draw(struct particle_bin* bin)
{
	for (size_t i = 0; i < bin->particles_used; i++)
	{
		const struct particle* const particle = bin->particles + i;

		particle->draw(current_timestamp - particle->start_timestamp, particle->seed);
	}
}

void particle_bin_update(struct particle_bin* bin)
{
	for (size_t i = 0; i < bin->particles_used; i++)
		if (bin->particles[i].end_timestamp <= current_timestamp)
			bin->particles[i--] = bin->particles[--bin->particles_used];
}

