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

-- return and expect ids
function manager:move(piece,zone)
	print(piece,zone)
--[[
	print("manager",self,"piece",piece.id)
	print("zone",zone.id)

	for k, v in pairs(zone.pieces) do	
		print(" ",k,v)
	end
--]]
end

function manager:vaild_moves(piece)
	local zone = self.pieces[piece].zone

	print("vaild_moves",self,piece,zone)
	
	return {0,3,6}
end

function manager:nonvalid_move(piece,zone)
	print("nonvalid_movemove",piece,zone)
end

for i = 0,2 do
	for j = 0,2 do	
		local color = 0

		if (i+j) % 2 == 0 then
			color = 255
		end

		manager:new_zone(j*3+i,"square",{x = 300+110*i,y = 300 +110*j,color={color,color,color}})
	end
end

--[[
manager:new_zone("0x0","square",{x=300,y=300,color={255,255,255}})
manager:new_zone("1x0","square",{x=400,y=300,color={0,0,0}})
manager:new_zone("0x1","square",{x=300,y=400,color={0,0,0}})
manager:new_zone("1x1","square",{x=400,y=400,color={255,255,255}})
--]]

manager:new_piece("id1","checker",{x=100,y=100})
manager:new_piece("id2","checker",{x=200,y=100})

local a = manager.pieces["id2"].zones

widgets.move(manager.pieces["id1"],nil)

test_button = button_new()
test_button:set_keyframe{x=1100,y=500}

function test_button.left_click()
	manager:move("id1",4)
end

collectgarbage("collect")

print("Boot Complete")