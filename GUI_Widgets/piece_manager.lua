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
	print("move",piece,zone)	
end

function manager:vaild_moves(piece)
	print("vaild_moves",self,piece,zone)

	local zone = self.pieces[piece].zone

	local defaults = {
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

	local output
	
	if zone then 
		output = defaults[zone]
	else
		output = {0,1,2,3,4,5,6,7,8}
	end

	local i = 1
    while (i <= #output) do
        if next(manager.zones[output[i]].pieces) then
			table.remove(output, i)
        else
           i = i + 1 
        end
    end

	return output
end

function manager:nonvalid_move(piece,zone)
	print("nonvalid_movemove",piece,zone)
end

test_button = button_new{x=1100,y=500,text="Manual Move"}

for i = 0,2 do
	for j = 0,2 do	
		local color = 0

		if (i+j) % 2 == 0 then
			color = 255
		end

		manager:new_zone(j*3+i,"square",{x = 300+110*i,y = 300 +110*j,color={color,color,color},camera=1})
	end
end

manager:new_piece("id1","checker",{x=410,y=200})
manager:new_piece("id2","checker",{x=200,y=100})

test_button = button_new{x=1100,y=500,text="Manual Move"}

function test_button.left_click()
	manager:move("id2",4)
end

lock_button = button_new{x=1100,y=700,text="Lock"}

function lock_button.left_click()
-- Not going to work since the engine isn't idle
-- I will fix after making a more robust state system
	widgets.lock(test_button)
	camera.push{x=100,y=100,timestamp = 1.0}
	scheduler.push(function() widgets.unlock() end, 1.0)
end

collectgarbage("collect")

contex_menu(function(x,y) print(x,y,"Context Menu Test") end)

print("Boot Complete")

