-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

--[[
prototype = prototype_new()
prototype:set_keyframe{x=1100,y=500}
--]]

material2 = material_test_new{effect=2,selection=0}
material2:set_keyframe{x=750,y=500}

material3 = material_test_new{effect=1,selection=0}
material3:set_keyframe{x=1000,y=500}

material1 = material_test_new{effect=3,selection=0}
material1:set_keyframe{x=500,y=500}

material4 = material_test_new{effect=4,selection=0}
material4:set_keyframe{x=1250,y=500}

print("post-boot complete")