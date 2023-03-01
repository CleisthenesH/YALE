-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

prototype = prototype_new()
prototype:set_keyframe{x=1100,y=500}

material = material_test_new{effect=3,selection=0}
material:set_keyframe{x=500,y=500}

print("post-boot complete")