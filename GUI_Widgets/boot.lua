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
	print(self)

	for k, v in pairs(self.pieces) do	
		print(k,v)
	end
end

manager:new_piece("id1","checker",{x=100,y=100})
manager:new_piece("id2","checker",{x=200,y=100})
manager:new_zone("zone","square",{x=300,y=300})

local a = manager.test

--widgets.move(manager.pieces["id1"],manager.zones["zone"])
widgets.move(manager.pieces["id1"],nil)

collectgarbage("collect")

print("Boot Complete")