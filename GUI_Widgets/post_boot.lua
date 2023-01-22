-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

control_rect = rectangle_new()
moving_rect = rectangle_new()
test_button = button_new()
slider = slider_new()

current_timestamp = current_time()

control_rect:set_keyframe{x=700,y=100}

circle = {}

for angle = 0.0,6.28318530718,6.28318530718/1000.0 do
	table.insert(circle,{x=400+100*math.cos(angle), y = 400 + 100*math.sin(angle), timestamp = current_timestamp + angle*0.5})
end

moving_rect:set_keyframes(circle)
circle = nil

moving_rect:enter_loop(6.28318530718/1000.0)

test_button:set_keyframe{x=700,y=300}
slider:set_keyframe{x=700,y=500}

function control_rect:left_click()
	destination = moving_rect.destination_keyframe

	moving_rect:interupt()
end

-- syntactic sugar for control_rec.right_click = function()
function control_rect:right_click()
-- TODO: Fix, I think it's just using a deprecated function
	if destination ~= nil then
		current_timestamp = current_time()
		destination.timestamp = current_timestamp + 1
		moving_rect:new_keyframe(destination)

		destination = nil
	else
		moving_rect = nil
		collectgarbage()
	end
end

piece_manager = piece_manager_new()	

piece_manager:new_piece("checker")
piece_manager:new_piece("checker")
piece_manager:new_piece("checker")
piece_manager:new_piece("checker")
piece_manager:new_zone("square")

print("pieces")
local x_pos = 700
for i,v in pairs(piece_manager.pieces) do
	v:set_keyframe{x=x_pos,y=700}
	x_pos = x_pos + 100
	print(i,v)
end

print("zones")
local x_pos = 700
for i,v in pairs(piece_manager.zones) do
	v:set_keyframe{x=x_pos,y=900}
	x_pos = x_pos + 100
	print(i,v)
end

print("post boot complete")

