-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

manager = board_manager_new{
	snap = true, 
	highlight = true, 
	transition = true,
}

-- return and expect ids
function manager:move(piece,zone)
	print("move",piece,zone)	
end

function manager:vaild_moves(piece)
	print("vaild_moves",self,piece)
end

function manager:nonvalid_move(piece,zone)
	print("nonvalid_movemove",piece,zone)
end

local function axial_to_world(q,r)
	local size = 50
	
	return 1.1*size*math.sqrt(3)*(q+0.5*r),1.1*size*1.5*r
end

axial_to_world(1,2)

for q = -1, 1 do
	for r = -1,1 do	
		if q ~= r then
		local dx, dy = axial_to_world(q,r)
		print(q,r,dx,dy)
		manager:new_zone(q+10*r,"tile",{x=300+dx,y=300+dy,camera = 1})
		end
	end
end

manager:new_piece("meeple","meeple",{x=410,y=200})

collectgarbage("collect")

print("Boot Complete")