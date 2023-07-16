-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

local item = scheduler.push(function() print("test 5") end, 5.0)
local item2 = scheduler.push(function() print("test 7") end, 5.0)

item2:change(2)

print("Boot Complete")