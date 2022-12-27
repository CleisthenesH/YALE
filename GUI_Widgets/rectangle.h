// Copyright 2022 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include <allegro5/allegro5.h>

struct rectangle
{
	struct widget_interface* widget_interface;
	double width, height;
	ALLEGRO_COLOR color;

	int cnt;
};
