// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"

// A simple card widget meant for experementing with the foiling workflow
// Not updated to the new lua paradigm
struct card
{
	struct widget_interface* widget_interface;
	char* name;
	char* effect;
	ALLEGRO_BITMAP* artwork;
};
