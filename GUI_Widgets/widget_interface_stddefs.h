// Copyright 2023 Kieran W Harvie. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
#pragma once

/*	Some optional preprocessor defintions to speed up widget devlopment.
 *
 *	Assumes the token WIDGET_TYPE has been defined such that:
 *      - There is a struct declared as "struct WIDGET_TYPE"
 *		- That this struct has the field "struct widget_interface* widget_interface"
 *		- That there is a varitable "struct widget_interface* widget_interface"
 *
 * Note: I tried preprocessors with unballanced {} to save some lines, 
 *	but they bugged out auto folding and syntax highlighting.
 */

#define WG_DECL(method) \
static void method(struct widget_interface* const widget_interface)

#define WG_DECL_CONST(method) \
static void method(const struct widget_interface* const widget_interface)

#define WG_DECL_DRAW WG_DECL_CONST(draw)
#define WG_DECL_MASK WG_DECL_CONST(mask)

#define _WG_DECL_NEW(type) int type ## _new(lua_State* L)
#define _WG_DECL_NEW_EXPANSION_BIND(type) _WG_DECL_NEW(type)
#define WG_DECL_NEW _WG_DECL_NEW_EXPANSION_BIND(WIDGET_TYPE)

#define WG_CAST \
struct WIDGET_TYPE* const WIDGET_TYPE = (struct WIDGET_TYPE* const)widget_interface->upcast; \

#define WG_CAST_CONST \
const struct WIDGET_TYPE* const WIDGET_TYPE = (const struct WIDGET_TYPE* const)widget_interface->upcast; \

#define WG_DIMENSIONS \
const double half_width = widget_interface->render_interface->half_width; \
const double half_height = widget_interface->render_interface->half_height; \

#define WG_CAST_DRAW \
WG_CAST_CONST \
WG_DIMENSIONS

#define WG_NEW \
struct WIDGET_TYPE * WIDGET_TYPE = malloc(sizeof(struct WIDGET_TYPE)); \
if (!WIDGET_TYPE) return 0; \
*WIDGET_TYPE = (struct WIDGET_TYPE)

#define _WG_NEW_HEADER(type) \
.widget_interface = widget_interface_new(L, type, &type ## _jump_table_entry)
#define _WG_NEW_HEADER_EXPANSION_BIND(type) _WG_NEW_HEADER(type)
#define WG_NEW_HEADER _WG_NEW_HEADER_EXPANSION_BIND(WIDGET_TYPE)

#define _WG_JMP_TBL(type) \
static const struct widget_jump_table type ## _jump_table_entry =
#define _WG_JMP_TBL_EXPANSION_BIND(type) _WG_JMP_TBL(type)
#define WG_JMP_TBL _WG_JMP_TBL_EXPANSION_BIND(WIDGET_TYPE)

//AAAAAAAAAAAAAAAAAA JUMP TABLE ENTRY DECL

#define WIDGET_MIN_DIMENSIONS(width,height) \
if (WIDGET_TYPE->widget_interface->render_interface->half_width == 0) \
WIDGET_TYPE->widget_interface->render_interface->half_width = width; \
if (WIDGET_TYPE ->widget_interface->render_interface->half_height == 0) \
WIDGET_TYPE->widget_interface->render_interface->half_height = height;

