// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// #define SCHEDULER_TESTING

#include "scheduler.h"

#include <stdlib.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

#ifdef SCHEDULER_TESTING
void stack_dump(lua_State*);
#endif

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
extern lua_State* main_lua_state;

// Heap Operations

// Get a node's children
static inline void heap_children(size_t parent, size_t* a, size_t* b)
{
	*a = 2 * parent + 1;
	*b = 2 * parent + 2;
}

// Get a node's parent
static inline size_t heap_parent(size_t child)
{
	return (child - 1) >> 1;
}

// Swap two nodes, doesn't maintain the heap property
static inline void heap_swap(size_t i, size_t j)
{
	struct scheduler_interface* tmp;

	tmp = heap[i];
	heap[i] = heap[j];
	heap[j] = tmp;
}

// The node has potentially decreased, maintain the heap property by moving it up
static inline void heap_heapify_up(size_t node)
{
	while (node > 0)
	{
		size_t parent = heap_parent(node);

		if (heap[node]->timestamp > heap[parent]->timestamp)
			break;

		heap_swap(node, parent);
		node = parent;
	}
}

// The node has potentially increased, maintain the heap property by moving it down
static inline void heap_heapify_down(size_t node)
{
	size_t parent, child_left, child_right;

	parent = node;
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

	if (child_left == used - 1 && heap[child_left]->timestamp < heap[parent]->timestamp)
		heap_swap(child_left, parent);

}

// The node has potentially changed, maintain the heap property by moving it
static inline void heap_heapify(size_t node)
{
	if (node == 0)
	{
		heap_heapify_down(node);
		return;
	}

	const size_t parent = heap_parent(node);

	if (heap[parent]->timestamp > heap[node]->timestamp)
	{
		heap_heapify_up(node);
		return;
	}

	heap_heapify_down(node);
}

// Insert an item into the heap
static inline void heap_insert(struct scheduler_interface* item)
{
	heap[used++] = item;

	heap_heapify_up(used - 1);
}

// Remove and item from the heap, it will be located at heap[used]
static inline void heap_remove(size_t node)
{
	heap_swap(node, --used);

	// TODO: We could check the timestamp change here call heap_heapify_{left,right} directly.
	// Saving comparison in heap_heapify.
	heap_heapify(node);
}

// Remove the minimum item from the heap, it will be located at heap[used]
static inline void heap_pop()
{
	if (used <= 1)
		return;

	heap_swap(0, --used);

	heap_heapify_down(0);
}

// Find and item in the heap
static size_t heap_find(size_t node, struct scheduler_interface* item)
{
	if (item == heap[node])
		return node;

	if (item->timestamp < heap[node]->timestamp)
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

#ifdef SCHEDULER_TESTING
static void scheduler_dump()
{
	for (size_t i = 0; i < used; i++)
	{
		struct scheduler_interface* item = heap[i];
		printf("%zu\t%lf\t%p\t%p\n", i, item->timestamp, item->funct, item->data);
	}
}
#endif

// Lua Interface

struct scheduler_item_lua
{
	struct scheduler_interface* scheduler_interface;
};

static void scheduler_call_wapper(void* data)
{
	const struct scheduler_item_lua* item = (struct scheduler_item_lua*) data;

	lua_getglobal(main_lua_state, "scheduler");
	lua_rawgetp(main_lua_state, -1, item);
	lua_getiuservalue(main_lua_state, -1, 1);

	lua_call(main_lua_state, 0, 0);

	lua_pushnil(main_lua_state);
	lua_rawsetp(main_lua_state, -3, item);

	lua_pop(main_lua_state, 2);
}

static int scheduler_push_lua(lua_State* L)
{
	const double timestamp = luaL_checknumber(L, -1);
	lua_pop(L, 1);

	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return 0;
	}

	struct scheduler_item_lua* item = lua_newuserdatauv(L, sizeof(struct scheduler_item_lua), 1);

	if (!item)
	{
		lua_pop(L, 2);
		return 0;
	}

	lua_rotate(L, -2, 1);
	lua_setiuservalue(L, -2, 1);

	item->scheduler_interface = scheduler_push(timestamp, scheduler_call_wapper, item);

	// Set metatable
	luaL_getmetatable(L, "scheule_item_mt");
	lua_setmetatable(L, -2);

	// Add to scheduuler table
	lua_getglobal(L, "scheduler");
	lua_pushvalue(L, -2);
	lua_rawsetp(L, -2, item);

	lua_pop(L, 1);

	return 1;
}

static int scheduler_remove_lua(lua_State* L)
{
	const struct scheduler_item_lua* item = (struct scheduler_item_lua*) luaL_checkudata(L, -1, "scheule_item_mt");
	scheduler_pop(item->scheduler_interface);

	lua_getglobal(L, "scheduler");
	lua_pushnil(L);
	lua_rawsetp(L, -2, item);

	lua_pop(L, 2);

	return 0;
}

static int scheduler_change_lua(lua_State* L)
{
	struct scheduler_item_lua* item = (struct scheduler_item_lua*) luaL_checkudata(L, -2, "scheule_item_mt");
	const double time = luaL_checknumber(L, -1);

	scheduler_change_timestamp(item->scheduler_interface, time, 0);

	lua_pop(L, 2);

	return 0;
}

static const luaL_Reg scheduler_index[] = {
	{"push",scheduler_push_lua},
	{NULL,NULL}
};

static const luaL_Reg item_index[] = {
	{"remove",scheduler_remove_lua},
	{"change",scheduler_change_lua},
	{NULL,NULL}
};

// Private Scheduler Interface

// Free a schedyler node, doesn't maintain heap property
static inline void scheduler_free_node(size_t node)
{
	free(heap[node]);
	heap[node] = NULL;
}

// Process a time change
void scheduler_process()
{
	while (heap[0] && heap[0]->timestamp < current_timestamp)
	{
		if (heap[0]->funct)
			heap[0]->funct(heap[0]->data);

		scheduler_free_node(0);

		heap_pop();
	}
}

#ifdef SCHEDULER_TESTING
static void test(void* _)
{
	printf("%lf\n", current_timestamp);
}
#endif

// Initalize the scheduler
void scheduler_init()
{
	heap = malloc(sizeof(struct scheduler_interface*));

	if (!heap)
	{
		printf("Unable to initalize scheduler heap\n");
		return;
	}

	heap[0] = NULL;

	allocated = 1;
	used = 0;

	lua_newtable(main_lua_state);

	// Set metatable
	lua_newtable(main_lua_state);

	lua_pushvalue(main_lua_state, -1);
	luaL_setfuncs(main_lua_state, scheduler_index, 0);
	lua_setfield(main_lua_state, -2, "__index");

	// Set mode
	lua_pushstring(main_lua_state, "k");
	lua_setfield(main_lua_state, -2, "__mode");

	lua_setmetatable(main_lua_state, -2);

	lua_setglobal(main_lua_state, "scheduler");

	// Make schedule item metable
	luaL_newmetatable(main_lua_state, "scheule_item_mt");
	lua_pushvalue(main_lua_state, -1);
	lua_setfield(main_lua_state, -2, "__index");
	luaL_setfuncs(main_lua_state, item_index, 0);

	lua_pop(main_lua_state, 1);

#ifdef SCHEDULER_TESTING
	printf("Stack after scheduler init: ");
	stack_dump(main_lua_state);

	struct scheduler_interface* handle = scheduler_push(2, test, NULL);
	scheduler_push(4, test, NULL);
	scheduler_push(6, test, NULL);

	scheduler_change_timestamp(handle, 10, 0);
#endif
}

// Public Scheduler Interface

// Push an item into the scheduler, maintains the heap property
struct scheduler_interface* scheduler_push(double timestamp, void(*funct)(void*), void* data)
{
	if (allocated <= used)
	{
		const size_t new_cnt = 2*allocated +1;

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

// Remove an item from the scheudler, maintinas the heap property
void scheduler_pop(struct scheduler_interface* item)
{
	const size_t node = heap_find(0, item);

	if (node == 0)
	{
		if (used >= 1 && item == heap[0])
		{
			scheduler_free_node(0);
			heap_pop();
		}

		return;
	}

	heap_remove(node);
	scheduler_free_node(used);
}

// Change the timestap of an item, maintains the heap property
void scheduler_change_timestamp(struct scheduler_interface* item, double time, int flag)
{
	const size_t node = heap_find(0, item);

	if (node == 0 && (used == 0 || item != heap[0]))
		return;

	item->timestamp += time;
	heap_heapify(node);
}
