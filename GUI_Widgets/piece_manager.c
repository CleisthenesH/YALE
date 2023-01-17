#include "piece_manager.h"
#include "widget_interface.h"

#include <stdint.h>
#include <stdbool.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

extern lua_State* const main_lua_state;

struct game_piece_jump_table
{
	void (*gc)(struct game_piece* const);
	void (*draw)(const struct game_piece* const);
	void (*mask)(const struct game_piece* const);
	// maybe add a function that pushes custom relevent to dragable context to the stack
};

struct game_zone_jump_table
{
	void (*gc)(struct game_piece* const);
	void (*draw)(const struct game_piece* const);
	void (*mask)(const struct game_piece* const);
};

struct game_piece
{
	struct widget_interface* widget_interface;
	const struct game_piece_jump_table* jump_table;

	struct game_zone* location;
	struct piece_manager* state;

	uint8_t type;
	uint8_t subtype;
	uint8_t owner;
	uint8_t controler;

	int is_dragable_funct_ref;
	int valild_zones_funct_ref;
};

struct game_zone
{
	struct widget_interface* widget_interface;
	struct widget_jump_table* jump_table;

	uint8_t owner;
	uint8_t controler;
	uint8_t type;		// Like inplay, inhand
	uint8_t subtype;	// Like a counter

	bool droppable_zone;
};

struct piece_manager
{
	struct game_zone* zones;
	size_t zones_allocated;
	size_t zones_used;

	struct game_piece* pieces;
	size_t pieces_allocated;
	size_t pieces_used;
} ;

// game_piece function

static void piece_gc(struct widget_interface* const widget)
{
	 struct game_piece* const game_piece = (struct game_piece*) widget->upcast;
	 game_piece->jump_table->gc(game_piece);
}

static void piece_draw(const struct widget_interface* const widget)
{
	const struct game_piece* const game_piece = (struct game_piece*)widget->upcast;
	game_piece->jump_table->draw(game_piece);
}

static void piece_mask(const struct widget_interface* const widget)
{
	const struct game_piece* const game_piece = (struct game_piece*)widget->upcast;
	game_piece->jump_table->mask(game_piece);
}

static void piece_hover_start(struct widget_interface* const widget)
{
	struct game_piece* const game_piece = (struct game_piece*)widget->upcast;

	// Check if the game_piece is draggable
	if (game_piece->is_dragable_funct_ref != LUA_REFNIL)
	{
		lua_rawgeti(main_lua_state, LUA_REGISTRYINDEX, game_piece->is_dragable_funct_ref); 

		lua_pushinteger(main_lua_state, game_piece->type);
		lua_pushinteger(main_lua_state, game_piece->subtype);
		lua_pushinteger(main_lua_state, game_piece->controler);
		lua_pushinteger(main_lua_state, game_piece->owner);

		if (lua_pcall(main_lua_state, 4, 1, 0) == LUA_OK && !lua_isnil(main_lua_state, -1))
			widget->is_draggable = (luaL_checkinteger(main_lua_state,-1) != 0);
		// TODO: handle errors
	}

	// Update snappable zones
}

static void piece_hover_end(const struct widget_interface* const widget)
{
	// some type of default snap and dragable ness?
}

// game_zone functions

static void zone_gc(struct widget_interface* const widget)
{
	struct game_zone* const game_zone = (struct game_zone*)widget->upcast;
	game_zone->jump_table->gc(game_zone);
}

static void zone_draw(const struct widget_interface* const widget)
{
	struct game_zone* const game_zone = (struct game_zone*)widget->upcast;
	game_zone->jump_table->draw(game_zone);
}

static void zone_mask(const struct widget_interface* const widget)
{
	struct game_zone* const game_zone = (struct game_zone*)widget->upcast;
	game_zone->jump_table->mask(game_zone);
}

static void zone_drag_end_drop(struct widget_interface* const zone, struct widget_interface* const piece)
{

}

// wiring up jump tables

static const struct widget_jump_table game_piece_to_widget_table =
{
	.gc = piece_gc,
	.draw = piece_draw,
	.mask = piece_mask,
	.hover_start = piece_hover_start,
	.hover_end = piece_hover_end,
};

static const struct widget_jump_table game_zone_to_widget_table =
{
	.gc = zone_gc,
	.draw = zone_draw,
	.mask = zone_mask,
	.drag_end_drop = zone_drag_end_drop,
};

// piece_manager methods

static int new_zone(lua_State* L)
{
	printf("new_zone\n");
	return 0;
}

static int piece_manager_new(lua_State* L)
{
	// TODO: implements hints
	struct piece_manager* const piece_manager = lua_newuserdata(L, sizeof(struct piece_manager));

	if (!piece_manager)
		return 0;

	*piece_manager = (struct piece_manager)
	{
		.pieces = NULL,
		.pieces_allocated = 0,
		.pieces_used = 0,

		.zones = NULL,
		.zones_allocated = 0,
		.zones_used = 0,
	};

	luaL_getmetatable(L, "piece_manager_mt");
	lua_setmetatable(L, -2);

	return 1;
}

void piece_manager_setglobal(lua_State* L)
{
	// global contructor
	lua_pushcfunction(L, piece_manager_new);
	lua_setglobal(L, "piece_manager_new");

	// Make the pieces manager meta table
	luaL_newmetatable(L, "piece_manager_mt");

	const struct luaL_Reg meta_table[] = {
		{"new_zone",new_zone},
		{NULL,NULL}
	};

	luaL_setfuncs(L, meta_table, 0);
}
