-- Copyright 2022 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

control_rect = widget_engine.rectangle()
moving_rect = widget_engine.rectangle()

current_timestamp = widget_engine.current_time()

moving_rect:set_keyframe{x=100,y=100}
control_rect:set_keyframe{x=700,y=100}

for angle = 0,6.28318530718,6.28318530718/1000.0 do
	moving_rect:new_keyframe{x=400+100*math.cos(angle), y = 400 + 100*math.sin(angle), timestamp = current_timestamp + angle}
end

control_rect:left_click(
	function()
		destination = moving_rect:destination_keyframe()
		moving_rect:interupt()
	end
)

control_rect:right_click(
	function()
		if destination ~= nil then
			current_timestamp = widget_engine.current_time()
			destination["timestamp"] = current_timestamp + 1
			moving_rect:new_keyframe(destination)

			destination = nil
		else
			moving_rect = nil
			collectgarbage()
		end
	end
)

card = widget_engine.card()

card:set_keyframe{x=800,y=500}
