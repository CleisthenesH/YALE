// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// This really isn't necessary at this time.
// String hashing is only used in scripting and this implementation isn't that robust.

#pragma once

#include <stdint.h>

struct hash_data;

struct hash_data* calc_hash_data(const char* [], uint8_t*);
uint8_t hash(struct hash_data*, const char*);

// add a safe call or lua integration


struct hash_table;

struct hash_table* hash_table_new(const char* []);
uint8_t hash_table_get(struct hash_table*, const char*);


