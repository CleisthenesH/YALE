-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- sketching how I want the game_state object to work

--[[
	pieces_mananger is designed to easily manage two type of widgets
		zones and pieces.
	these widgets have extra standard fields (type, subtype,owner,controler).

	when the drag of a piece starts a callback occours to calculate which zones the pice and end (and snap too).
	and a callback occours when a piece is dropped in that zone
--]]

game_board = piece_manager_new()

-- easily make new pices and zones
game_board:new_zone("main")
-- pieces need to be in a zone
game_board:new_piece("pawn")

control_rect = widget_engine.rectangle()
moving_rect = widget_engine.rectangle()
test_button = widget_engine.button()
slider = widget_engine.slider()

current_timestamp = widget_engine.current_time()

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
		current_timestamp = widget_engine.current_time()
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

test_square = piece_manager:new_zone("square")
test_square:set_keyframe{x=700,y=800}

for i,v in pairs(piece_manager.pieces) do
	v:set_keyframe{x=700+100*i,y=700}
	print(i,v)
end

print(piece_manager.pieces)

print("post boot complete")

--[[
card = widget_engine.card()
card:set_keyframe{x=800,y=500}
--]]


