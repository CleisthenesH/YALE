-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

manager = board_manager_new{
	snap = true, 
	highlight = true, 
	transition = true, 
	allowed_zones = {"square"}, 
	allowed_pieces = {"checker"}}

function manager:move(piece,zone)
	print("manager",self,"piece",piece.id)
	print("zone",zone.id)

	for k, v in pairs(zone.pieces) do	
		print(" ",k,v)
	end
end

function manager:vaild_moves(piece)
	print("vaild_moves",self,piece)
	
	local table = self.zones

	return table
end

manager:new_piece("id1","checker",{x=100,y=100})
manager:new_piece("id2","checker",{x=200,y=100})
manager:new_zone("0x0","square",{x=300,y=300,color={255,255,255}})
manager:new_zone("1x0","square",{x=400,y=300,color={0,0,0}})
manager:new_zone("0x1","square",{x=300,y=400,color={0,0,0}})
manager:new_zone("1x1","square",{x=400,y=400,color={255,255,255}})

--local a = manager.test

widgets.move(manager.pieces["id1"],nil)

collectgarbage("collect")

print("Boot Complete")