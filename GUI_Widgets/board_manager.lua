-- This is a sketch of how I might make the board manager api.
-- Everything here is subject to change.
	
-- First we construct the new board manager
-- There are several automatic features that can be turned on or off.
-- There should also be way of passing the original board state, but the table format of the table will come later
manager = board_manager_new{
	snap = false, 
	highlight = false, 
	transition = true, 
	allowed_zones = {"square"}, 
	allowed_pieces = {"checker"}}

-- This is one of the callbacks the manager gives.
-- When the manager needs to know which moves are vaild for a piece
--
-- Input: piece identifier
-- Output: an array of valid zone identifiers
function manager:vaild_moves(piece)
end

-- This is the second callback
-- It is called after the player has selected a move
--
-- Input: piece identifier and zone identifier
-- Output: None
function manager:move(piece, zone)
end

-- I might add a second method for when the user attemps and invalid move
function manager:invaild_move(piece, zone)
end

-- The method for making a new zone
--
-- Input: Manditory id and type, optional table to be feed to the widget contructor.
-- Output: Widget interface for that zone.
mananger:new_zone("id1", "square", {x=100,y=200})

-- The methods for making a new piece
--
-- Input: Manditory id and type, optional table to be feed to the widget contructor.
-- Output: Widget interface for that zone.
special_checker = mananger:new_piece("id2", "checker", nil)


-- This is an example of the format for feeding a board position into the manager.
-- The outer element represents zones and contains two manditory fields, the zones id and type.
-- And optional fields, a table to be fed to the widget constructor, and a record style list of pieces
--
-- The inner element represents the pieces with the same two manditory fields, type and id.
board = {
	{id = "0x0", type = "square", widget = {x = 100, y = 100}, 
		{id = "red", type = "checker"}
	},
	{id = "0x1", type = "square", widget = {x = 100, y = 210}},
	{id = "1x0", type = "square", widget = {x = 210, y = 100}},
	{id = "1x1", type = "square", widget = {x = 210, y = 210}}
}

-- When a board is read into the manager if a board with that id and type is present that widget won't get remaid but will instead get reused.
-- 	(What is the difference to the lua writer? I guess that references to eachother still work? Should the lua writer care about such thing?)
manager:read(board)

-- This is just a method for getting the current board.
-- It is the same format as the above method, but only the manditory fields. (Except maybe the keyframe?)
output_board = manager.board

-- There are similar methods for setting auto_snap, auto_highlight. And for reading the pieces and zones associated with the board.
