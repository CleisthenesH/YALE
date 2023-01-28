-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

piece_manager = piece_manager_new()

for i = 0,2 do
	for j = 0,2 do
		local zone = piece_manager:new_zone("square")
		zone:set_keyframe{x=500+i*110,y=500+j*110}
		zone.payload = (i+j)%2
	end
end

piece_manager:new_piece("checker"):set_keyframe{x=300, y=100}

local checker = piece_manager:new_piece("checker")
checker:set_keyframe{x=100, y=100}
--checker:new_keyframe{x=900, y=100, timestamp = current_time()+1}
--checker:new_keyframe{x=400, y=200, timestamp = current_time()+2}
--checker:enter_loop(1)

function piece_manager.pre_move(manager, piece)
	local a = {}

	for k,v in pairs(manager.zones) do
		if(v.payload == 0) and (next(v.pieces) == nil) then
			a[k] = v
		end
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

