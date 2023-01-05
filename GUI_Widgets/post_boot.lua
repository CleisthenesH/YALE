-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

control_rect = widget_engine.rectangle()
moving_rect = widget_engine.rectangle()

current_timestamp = widget_engine.current_time()

control_rect:set_keyframe{x=700,y=100}

circle = {}

for angle = 0.0,6.28318530718,6.28318530718/1000.0 do
	table.insert(circle,{x=400+100*math.cos(angle), y = 400 + 100*math.sin(angle), timestamp = current_timestamp + angle*0.5})
end

moving_rect:set_keyframes(circle)
circle = nil

moving_rect:enter_loop(6.28318530718/1000.0)

function control_rect:left_click()
	destination = moving_rect.destination_keyframe

	moving_rect:interupt()
	
	throw["error"]()
end

function control_rect:right_click()
	if destination ~= nil then
		current_timestamp = widget_engine.current_time()
		destination.timestamp = current_timestamp + 1
		moving_rect:new_keyframe(destination)

		destination = nil
	else
		moving_rect = nil
		collectgarbage()
	end
end

print("post boot complete")

--[[
card = widget_engine.card()
card:set_keyframe{x=800,y=500}
--]]


