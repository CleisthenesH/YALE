-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- sketching how I want the game_state object to work

--[[
	pieces_mananger is designed to easily manage two type of widgets
		zones and pieces.
	these widgets have extra standard fields (type, subtype,owner,controler).

	when the drag of a piece starts a callback occours to calculate which zones the pice and end (and snap too).
	and a callback occours when a piece is dropped in that zone
--]]

game_board = piece_manager_new()

-- easily make new pices and zones
game_board:new_zone("main")
-- pieces need to be in a zone
game_board:new_piece("pawn")

