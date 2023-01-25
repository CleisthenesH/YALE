-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

piece_manager = piece_manager_new()	

piece_manager:new_zone("square"):set_keyframe{x=500, y=500}
piece_manager:new_zone("square"):set_keyframe{x=700, y=500}
piece_manager:new_zone("square"):set_keyframe{x=500, y=700}
piece_manager:new_zone("square"):set_keyframe{x=700, y=700}

local checker = piece_manager:new_piece("checker")
checker:set_keyframe{x=100, y=100}
--checker:new_keyframe{x=900, y=100, timestamp = current_time()+1}
--checker:new_keyframe{x=400, y=200, timestamp = current_time()+2}
--checker:enter_loop(1)

function piece_manager.pre_move(manager, piece)
	local a = {}
	local idx = 0

	for k,v in pairs(manager.zones) do
		if(idx % 2 == 0) then
			a[k] = v
		end

		idx = idx + 1
	end
	
	return a
end

function piece_manager.post_move(manager, piece, zone)
	destination = zone.destination_keyframe
	current_timestamp = current_time()

	destination.timestamp = current_timestamp + 0.1

	piece:interupt()
	piece:new_keyframe(destination)
end

print("post boot complete")

