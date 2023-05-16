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

	if zone == nil then
		return {0,1,2,3,4,5,6,7,8}
	end

	print("vaild_moves",self,piece,zone)

	local table = {
		[0] = {1,3},
		[1] = {0,2,4},
		[2] = {1,5},
		[3] = {0,4,6},
		[4] = {1,3,5,7},
		[5] = {2,4,8},
		[6] = {3,7},
		[7] = {4,6,8},
		[8] = {5,7}
	}

	local output = table[zone]

	return table[zone]
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

manager:new_piece("id1","checker",{x=410,y=200})
manager:new_piece("id2","checker",{x=200,y=100})

local a = manager.pieces["id2"].zones

widgets.move(manager.pieces["id1"],nil)

test_button = button_new{x=1100,y=500,text="Manual Move"}

function test_button.left_click()
	manager:move("id1",4)
end

collectgarbage("collect")

local item = scheduler.push(function() print("test 5") end, 5.0)
local item2 = scheduler.push(function() print("test 7") end, 5.0)

item2:change(2)


print("Boot Complete")