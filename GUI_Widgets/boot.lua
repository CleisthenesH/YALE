-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

--[[
material2 = material_test_new{x=750, y=500, effect=2, selection=0}

material3 = material_test_new{x=1000, y=500, effect=1, selection=0}

material1 = material_test_new{x=500, y=500, effect=3, selection=0}
--]]

--material4 = material_test_new{x=1250, y=500, effect=4, selection=0}

button = button_new{x=100, y=100}

--prototype = prototype_new{x=500, y=500}

print("hit 1")
piece_manager = piece_manager_new()

print("hit 2")
local arbitary_zone = nil

print("hit 3")
for i = 0,2 do
print("hit 3.1")
	for j = 0,2 do
	print("hit 3.2")
		local zone = piece_manager:new_zone("square")
		print("hit 3.3")
		zone:set_keyframe{x=300+110*i,y=300+110*j}

		print("hit 3.4")
		zone.payload = i+j

		print("hit 3.2")
		arbitary_zone = zone
	end
end

print("hit 4")

local checker = piece_manager:new_piece("checker")
checker:set_keyframe{x=900,y=500}

print("hit 5")

function piece_manager.vaild_moves(manager, piece)
	local a = {}

	for k,v in pairs(manager.zones) do
		if (v.payload % 2 == 0) and (next(v.pieces) == nil) then
			a[k] = v
		end
	end
	
	return a
end

test_button = button_new()
test_button:set_keyframe{x=1100,y=500}

function test_button.left_click()
	piece_manager:move(checker,arbitary_zone)
end

print("Boot Complete")