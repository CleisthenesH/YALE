-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

manager = board_manager_new{
	snap = true, 
	highlight = true, 
	transition = true,
}

function hash_cord(q,r)
	return q+100*r
end

function unhash_cord(hash)
	local q = hash % 100
	
	if q > 50 then
		q = q- 100
	end

	local r = (hash - q)//100

	if r > 50 then
		r = r-100
	end

	return q,r
end

function get_neighbours(zone)
	local q,r = unhash_cord(zone)

	local output = {}

	for dq = -1, 1 do
		for dr = -1, 1 do
			if math.abs(dr+dq) <= 1 and math.abs(q+dq+r+dr) < 4 then
				output[#output+1] = hash_cord(q+dq,r+dr)
			end
		end
	end

	return output
end

-- return and expect ids
function manager:move(piece,zone)
	print("move",piece,zone)	
end

function manager:vaild_moves(piece,zone)
	print("vaild_moves",self,piece,zone)
	manager.zones[zone].tile = "camp"

	return get_neighbours(zone)
end

function manager:nonvalid_move(piece,zone)
	print("nonvalid_movemove",piece,zone)
end

local function axial_to_world(q,r)
	local size = 50
	
	return 1.1*size*math.sqrt(3)*(q+0.5*r),1.1*size*1.5*r
end

axial_to_world(1,2)

for q = -3,3 do
	for r = -3,3 do	
		if math.abs(q+r) < 4 then
			local dx, dy = axial_to_world(q,r)
			manager:new_zone(q+100*r,"tile",{
				x=300+dx,y=300+dy,
				camera = 1,
				team =  q < 0 
				and "red" 
				or "blue",
				tile = "hills"})
		end
	end
end

manager:new_piece("meeple","meeple",{x=410,y=200})
manager:move("meeple",101)

collectgarbage("collect")

print("Boot Complete")