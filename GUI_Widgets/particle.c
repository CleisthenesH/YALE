// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "particle.h"
#include "thread_pool.h"
#include <stdlib.h>

extern double current_timestamp;

struct particle
{
	const struct particle_jumptable* jumptable;
	void* data;
	double start_timestamp;
	double end_timestamp;
};

struct particle_bin
{
	struct particle* particles;
	size_t particles_allocated;
	size_t particles_used;
};

static struct particle_bin** list;
static size_t allocated;
static size_t used;

struct particle_bin* particle_bin_new(size_t inital_size)
{
	if (allocated <= used)
	{
		const size_t new_cnt = 2 * allocated + 1;

		struct particle_bin** memsafe_hande = realloc(list, new_cnt * sizeof(struct particle_bin*));

		if (!memsafe_hande)
			return NULL;

		list = memsafe_hande;
		allocated = new_cnt;
	}

	struct particle_bin* bin = list[used++] = malloc(sizeof(struct particle_bin));

	if (!bin)
	{
		used--;
		return NULL;
	}

	*bin = (struct particle_bin)
	{
		.particles = malloc(inital_size * sizeof(struct particle)),
		.particles_allocated = inital_size,
		.particles_used = 0
	};

	return bin;
}

void particle_bin_append(struct particle_bin* bin, const struct particle_jumptable* jumptable, void* data, double end_timestamp)
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

	particle->data = data;
	particle->jumptable = jumptable;
	particle->end_timestamp = current_timestamp + end_timestamp;
	particle->start_timestamp = current_timestamp;
}

void particle_bin_draw(struct particle_bin* bin)
{
	for (size_t i = 0; i < bin->particles_used; i++)
	{
		const struct particle* const particle = bin->particles + i;
		particle->jumptable->draw(particle->data,current_timestamp - particle->start_timestamp);
	}
}

void particle_engine_init()
{
	list = NULL;
	used = 0;
	allocated = 0;
}

static void particle_update_work(struct particle* particle)
{
	particle->jumptable->update(particle->data, current_timestamp - particle->start_timestamp);
}

static void particle_bin_update_work(struct particle_bin* bin)
{
	if (0)
	{
		for (size_t i = 0; i < bin->particles_used; i++)
			if (bin->particles[i].end_timestamp <= current_timestamp)
			{
				if (bin->particles[i].jumptable->gc)
					thread_pool_push(bin->particles[i].jumptable->gc, bin->particles[i].data);

				bin->particles[i--] = bin->particles[--bin->particles_used];
			}
			else
				if (bin->particles[i].jumptable->update)
					thread_pool_push(particle_update_work, &bin->particles[i]);
	}
	else
	{
		for (size_t i = 0; i < bin->particles_used; i++)
		{
			struct particle* const particle = &bin->particles[i];

			if (particle->end_timestamp <= current_timestamp)
			{
				if (particle->jumptable->gc)
					particle->jumptable->gc(particle->data);


				bin->particles[i--] = bin->particles[--bin->particles_used];
			}
			else
				if (particle->jumptable->update)
					particle->jumptable->update(particle->data, current_timestamp - particle->start_timestamp);
		}
	}
}

struct work_queue* particle_engine_update()
{
	struct work_queue* work_queue = work_queue_create();

	for(size_t i = 0; i< used; i++)
		if(list[i]->particles_used > 0)
			work_queue_push(work_queue, particle_bin_update_work, list[i]);

	return work_queue;
}

