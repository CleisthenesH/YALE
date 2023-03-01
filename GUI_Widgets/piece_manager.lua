-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

piece_manager = piece_manager_new()

local arbitary_zone = nil

for i = 0,2 do
	for j = 0,2 do
		local zone = piece_manager:new_zone("square")
		zone:set_keyframe{x=300+110*i,y=300+110*j}

		zone.payload = i+j

		arbitary_zone = zone
	end
end

local checker = piece_manager:new_piece("checker")
checker:set_keyframe{x=900,y=500}

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

print("post boot complete")

