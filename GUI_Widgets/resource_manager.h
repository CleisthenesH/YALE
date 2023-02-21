// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once

#include "resource_manager_ids.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>

ALLEGRO_FONT* resource_manager_font(enum font_id);
ALLEGRO_BITMAP* resource_manager_icon(enum icon_id);
ALLEGRO_BITMAP* resource_manager_character_art(enum character_art_id);

