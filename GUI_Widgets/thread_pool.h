// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#pragma once

struct work_queue;

struct work_queue* work_queue_create();
void work_queue_push(struct work_queue*, void(*)(void*), void*);
void work_queue_destroy(struct work_queue*);

void thread_pool_push(void (*)(void*), void*);
void thread_pool_concatenate(struct work_queue*);
void thread_pool_wait();
