// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "scheduler.h"

#include <stdlib.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

struct scheduler_interface
{
	double timestamp;
	void (*funct)(void*);
	void* data;
};

static struct scheduler_interface** heap;
static size_t allocated;
static size_t used;

extern double current_timestamp;

// Heap Operations

static inline void heap_children(size_t parent, size_t* a, size_t* b)
{
	*a = 2 * parent + 1;
	*b = 2 * parent + 2;
}

static inline size_t heap_parent(size_t child)
{
	return (child - 1) >> 1;
}

static inline void heap_swap(size_t i, size_t j)
{
	struct scheduler_interface* tmp;

	tmp = heap[i];
	heap[i] = heap[j];
	heap[j] = tmp;
}

static inline void heap_insert(struct scheduler_interface* item)
{
	size_t node = used;
	heap[used++] = item;

	while (node > 0)
	{
		size_t parent = heap_parent(node);

		if (heap[node]->timestamp > heap[parent]->timestamp)
			break;

		heap_swap(node, parent);
		node = parent;
	}
}

static inline void heap_pop()
{
	if (used <= 1)
		return;

	heap_swap(0, --used);

	size_t parent, child_left, child_right;

	parent = 0;
	heap_children(0, &child_left, &child_right);

	while (child_right <= used - 1)
	{
		size_t child_min = heap[child_left]->timestamp < heap[child_right]->timestamp ? child_left : child_right;

		if (heap[child_min]->timestamp > heap[parent]->timestamp)
			return;

		heap_swap(child_min, parent);
		parent = child_min;
		heap_children(parent, &child_left, &child_right);
	}

	if (child_left == used -1 && heap[child_left]->timestamp < heap[parent]->timestamp)
		heap_swap(child_left, parent);
}

static size_t heap_find(size_t node, struct scheduler_interface* item)
{
	if (item == heap[node])
		return node;

	if (item->timestamp > heap[node]->timestamp)
		return 0;

	size_t child_left, child_right;

	heap_children(node, &child_left, &child_right);

	if (child_right >= used)
		return 0;

	if (child_left == used - 1)
		return heap[child_left] == item ? child_left : 0;

	const size_t idx = heap_find(child_left, item);

	if (idx)
		return idx;

	return heap_find(child_right, item);
}

// Public Scheduler Interface

struct scheduler_interface* scheduler_push(double timestamp, void(*funct)(void*), void* data)
{
	if (allocated <= used)
	{
		const size_t new_cnt = 1.2 * allocated+1;

		struct scheduler_interface** memsafe_hande = realloc(heap, new_cnt * sizeof(struct scheduler_interface*));

		if (!memsafe_hande)
			return NULL;

		heap = memsafe_hande;
		allocated = new_cnt;
	}

	struct scheduler_interface* item = malloc(sizeof(struct scheduler_interface));

	if (!item)
		return NULL;

	*item = (struct scheduler_interface)
	{
		.timestamp = timestamp+current_timestamp,
		.funct = funct,
		.data = data
	};

	heap_insert(item);

	return item;
}

void scheduler_pop(struct scheduler_interface* item)
{
}

// Private Scheduler Interface

void scheduler_process()
{
	while (heap[0] && heap[0]->timestamp < current_timestamp)
	{
		if (heap[0]->funct)
			heap[0]->funct(heap[0]->data);

		free(heap[0]);
		heap[0] = NULL;

		heap_pop();
	}
}

static void test(void* _)
{
	printf("%lf\n", current_timestamp);
}

void scheduler_init()
{
	heap = NULL;

	allocated = 0;
	used = 0;

	scheduler_push(2, test, NULL);
	scheduler_push(4, test, NULL);
	scheduler_push(6, test, NULL);
}