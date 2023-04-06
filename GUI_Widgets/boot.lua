-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

manager = board_manager_new{
	snap = false, 
	highlight = false, 
	transition = true, 
	allowed_zones = {"square"}, 
	allowed_pieces = {"checker"}}

function manager:move(piece)
	print(self, piece)
end

a = manager.test

manager:new_piece("id1","checker",nil)

print("Boot Complete")