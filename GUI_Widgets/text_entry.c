// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#include "widget_interface.h"
#include "resource_manager.h"

#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_opengl.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>

extern ALLEGRO_EVENT current_event;

struct text_entry {
	struct widget_interface* widget_interface;

	enum {
		TEXT_ENTRY_IDLE,
		TEXT_ENTRY_HOVER,
		TEXT_ENTRY_ACTIVE
	} state;

	ALLEGRO_FONT* font;

	ALLEGRO_COLOR field_background;
	ALLEGRO_COLOR field_boarder;
	ALLEGRO_COLOR active_text_color;
	ALLEGRO_COLOR unactive_text_color;

	int field_width;

	size_t input_size;
	char input[256];
	char place_holder[256];

	void (*on_enter)(void*);
};

static void draw(const struct widget_interface* const widget)
{
	const struct text_entry* const text_entry = (const struct text_entry* const)widget->upcast;

	const double field_height = 20;
	const double text_left_padding = 10;

	// Clear the stencil buffer channels
	glEnable(GL_STENCIL_TEST);
	glStencilMask(0x03);

	// Set Stencil Function to set stencil channels to 1 
	glStencilFunc(GL_ALWAYS, 1, 0x03);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	al_draw_filled_rectangle(-text_entry->field_width, -field_height, text_entry->field_width, field_height, text_entry->field_background);

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	// Draw all the interea boarders (where stencil is 2)
	glStencilFunc(GL_EQUAL, 1, 0x03);

	if (text_entry->input_size == 0)
	{
		al_draw_text(text_entry->font, text_entry->unactive_text_color, -text_entry->field_width+ text_left_padding, -16, 0, text_entry->place_holder);
	}
	else {
		const auto text_width = al_get_text_width(text_entry->font, text_entry->input);

		if (text_width > 2 * text_entry->field_width- text_left_padding)
			al_draw_text(text_entry->font, text_entry->active_text_color, text_entry->field_width - text_width, -16, 0, text_entry->input);
		else
			al_draw_text(text_entry->font, text_entry->active_text_color, -text_entry->field_width + text_left_padding, -16, 0, text_entry->input);
	}

	glDisable(GL_STENCIL_TEST);

	al_draw_rectangle(-text_entry->field_width, -field_height, text_entry->field_width, field_height, text_entry->field_boarder, 2);
}

static void mask(const struct widget_interface* const widget)
{
	const struct text_entry* const text_entry = (const struct text_entry* const)widget->upcast;
	al_draw_filled_rectangle(-text_entry->field_width, -16, text_entry->field_width, 16, al_color_name("white"));
}

static void hover_start(struct widget_interface* widget)
{
	struct text_entry* const text_entry = (const struct text_entry* const)widget->upcast;

	if (text_entry->state == TEXT_ENTRY_IDLE)
	{
		text_entry->state = TEXT_ENTRY_HOVER;
	}
}

static void hover_end(struct widget_interface* widget)
{
	struct text_entry* const text_entry = (const struct text_entry* const)widget->upcast;
	if (text_entry->state == TEXT_ENTRY_HOVER)
	{
		text_entry->state = TEXT_ENTRY_IDLE;
	}
}

static void drag_start(struct widget_interface* widget)
{
	struct text_entry* const text_entry = (const struct text_entry* const)widget->upcast;
	text_entry->state = TEXT_ENTRY_ACTIVE;
}

// Can combine with the previous
static void left_click(struct widget_interface* widget)
{
	struct text_entry* const text_entry = (const struct text_entry* const)widget->upcast;
	text_entry->state = TEXT_ENTRY_ACTIVE;
}

static void click_off(struct widget_interface* widget)
{
	struct text_entry* const text_entry = (const struct text_entry* const)widget->upcast;
	text_entry->state = TEXT_ENTRY_IDLE;
}

static void event_handler(struct widget_interface* widget)
{
	struct text_entry* const text_entry = (const struct text_entry* const)widget->upcast;
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

static const struct widget_jump_table text_entry_jump_table_entry =
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

int text_entry_new(lua_State* L)
{
	struct text_entry* ptr = malloc(sizeof(struct text_entry));

	if (!ptr)
		return 0;

	*ptr = (struct text_entry){
		.widget_interface = widget_interface_new(L,ptr,&text_entry_jump_table_entry),
		.state = TEXT_ENTRY_IDLE,
		.input_size = 0,
		.input = "",
		.place_holder = "Click to enter text.",
		.field_width = 300,

		.font = resource_manager_font(FONT_ID_SHINYPEABERRY),

		.field_background = al_color_name("pink"),
		.field_boarder = al_color_name("black"),
		.active_text_color = al_color_name("white"),
		.unactive_text_color = al_color_name("grey")
	};

	return 1;
}

