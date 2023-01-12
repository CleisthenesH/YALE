// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#pragma once

#include <stdint.h>

struct hash_data;

struct hash_data* calc_hash_data(const char* [], uint8_t*);
uint8_t hash(struct hash_data*, const char*);