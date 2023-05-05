// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include "resource_manager.h"
#include "widget_style_sheet.h"

#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_opengl.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>

extern ALLEGRO_EVENT current_event;

#define WIDGET_TYPE text_entry

struct text_entry {
	struct widget_interface* widget_interface;

	enum {
		TEXT_ENTRY_IDLE,
		TEXT_ENTRY_HOVER,
		TEXT_ENTRY_ACTIVE
	} state;

	ALLEGRO_FONT* font;

	size_t input_size;
	char input[256];
	char place_holder[256];

	void (*on_enter)(void*);
};

WG_DECL_DRAW
{
	WG_CAST_DRAW
	const double text_left_padding = 10;

	// Clear the stencil buffer channels
	glEnable(GL_STENCIL_TEST);
	glStencilMask(0x03);

	// Set Stencil Function to set stencil channels to 1 
	glStencilFunc(GL_ALWAYS, 1, 0x03);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	al_draw_filled_rounded_rectangle(-half_width, -half_height,
		 half_width, half_height,
		primary_pallet.edge_radius, primary_pallet.edge_radius,
		primary_pallet.recess);

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	// Draw all the interea boarders (where stencil is 2)
	glStencilFunc(GL_EQUAL, 1, 0x03);

	if (text_entry->input_size == 0)
	{
		al_draw_text(text_entry->font, primary_pallet.deactivated,
			-half_width+ text_left_padding,-16, 0, 
			text_entry->place_holder);
	}
	else {
		const auto text_width = al_get_text_width(text_entry->font, text_entry->input);

		if (text_width > 2 * half_width- text_left_padding)
			al_draw_text(text_entry->font, primary_pallet.activated,
				half_width - text_width, -16,
				0, text_entry->input);
		else
			al_draw_text(text_entry->font, primary_pallet.activated,
				-half_width + text_left_padding, -16,
				0, text_entry->input);
	}

	glDisable(GL_STENCIL_TEST);

	al_draw_rounded_rectangle(-half_width, -half_height,
		half_width, half_height, 
		primary_pallet.edge_radius, primary_pallet.edge_radius,
		primary_pallet.edge, primary_pallet.edge_width);
}

WG_DECL_MASK
{
	WG_CAST_DRAW

	al_draw_filled_rectangle(-half_width, -half_height,
		half_width, half_height, 
		al_color_name("white"));
}

WG_DECL(hover_start)
{
	WG_CAST

	if (text_entry->state == TEXT_ENTRY_IDLE)
	{
		text_entry->state = TEXT_ENTRY_HOVER;
	}
}

WG_DECL(hover_end)
{
	WG_CAST
	if (text_entry->state == TEXT_ENTRY_HOVER)
	{
		text_entry->state = TEXT_ENTRY_IDLE;
	}
}

WG_DECL(drag_start)
{
	WG_CAST
	text_entry->state = TEXT_ENTRY_ACTIVE;
}

WG_DECL(left_click)
{
	WG_CAST
	text_entry->state = TEXT_ENTRY_ACTIVE;
}

WG_DECL(click_off)
{
	WG_CAST
	text_entry->state = TEXT_ENTRY_IDLE;
}

WG_DECL(event_handler)
{
	WG_CAST
	ALLEGRO_EVENT* ev = &current_event;

	if (text_entry->state != TEXT_ENTRY_ACTIVE)
		return;

	if (ev->type == ALLEGRO_EVENT_KEY_CHAR)
	{
		if (ev->keyboard.keycode == ALLEGRO_KEY_ENTER)
		{
			if (text_entry->on_enter)
				text_entry->on_enter(text_entry);
			return;
		}

		if (ev->keyboard.keycode == ALLEGRO_KEY_BACKSPACE)
		{
			if (text_entry->input_size > 0)
				text_entry->input[--text_entry->input_size] = '\0';

			return;
		}

		if (text_entry->input_size < 255)
		{
			int append_char = -1;

			if (ev->keyboard.keycode >= ALLEGRO_KEY_A && ev->keyboard.keycode <= ALLEGRO_KEY_Z)
				append_char = ((ev->keyboard.modifiers & ALLEGRO_KEYMOD_SHIFT) ? 'A' : 'a') +
				(ev->keyboard.keycode - ALLEGRO_KEY_A);

			if (ev->keyboard.keycode >= ALLEGRO_KEY_0 && ev->keyboard.keycode <= ALLEGRO_KEY_9 &&
				!(ev->keyboard.modifiers & ALLEGRO_KEYMOD_SHIFT))
				append_char = '0' + (ev->keyboard.keycode - ALLEGRO_KEY_0);

			if (ev->keyboard.keycode == ALLEGRO_KEY_SPACE)
				append_char = ' ';

			if (ev->keyboard.keycode == ALLEGRO_KEY_EQUALS)
				append_char = '=';

			if (ev->keyboard.keycode == ALLEGRO_KEY_OPENBRACE)
				append_char = (ev->keyboard.modifiers & ALLEGRO_KEYMOD_SHIFT) ? '[' : '{';

			if (ev->keyboard.keycode == ALLEGRO_KEY_CLOSEBRACE)
				append_char = (ev->keyboard.modifiers & ALLEGRO_KEYMOD_SHIFT) ? ']' : '}';

			if (ev->keyboard.keycode == ALLEGRO_KEY_SEMICOLON)
				append_char = ':';

			if (ev->keyboard.keycode == ALLEGRO_KEY_FULLSTOP)
				append_char = '.';

			if (ev->keyboard.keycode == ALLEGRO_KEY_COMMA)
				append_char = ',';

			if (ev->keyboard.keycode == ALLEGRO_KEY_QUOTE)
				append_char = '"';

			if (ev->keyboard.keycode == ALLEGRO_KEY_MINUS)
				append_char = (ev->keyboard.modifiers & ALLEGRO_KEYMOD_SHIFT) ? '_' : '-';

			if (ev->keyboard.keycode == ALLEGRO_KEY_9)
				append_char = (ev->keyboard.modifiers & ALLEGRO_KEYMOD_SHIFT) ? '(' : '9';

			if (ev->keyboard.keycode == ALLEGRO_KEY_0)
				append_char = (ev->keyboard.modifiers & ALLEGRO_KEYMOD_SHIFT) ? ')' : '0';

			if (append_char != -1)
			{
				text_entry->input[text_entry->input_size++] = append_char;
				text_entry->input[text_entry->input_size] = '\0';

				// wobble on entry goes here.
			}
		}
	}
}

WG_JMP_TBL
{
	.draw = draw,
	.mask = mask,
	.event_handler = event_handler,

	.left_click = left_click,
	.hover_start = hover_start,
	.hover_end = hover_end,
	.drag_start = drag_start,
	.click_off = click_off
};

WG_DECL_NEW
{
	WG_NEW
	{ 
		WG_NEW_HEADER,
		.state = TEXT_ENTRY_IDLE,
		.input_size = 0,
		.input = "",
		.place_holder = "Click to enter text.",

		.font = resource_manager_font(FONT_ID_SHINYPEABERRY),
	};
	
	WIDGET_MIN_DIMENSIONS(300,20)
	
	return 1;
}
