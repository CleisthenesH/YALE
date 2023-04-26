-- Copyright 2023 Kieran W Harvie. All rights reserved.
-- Use of this source code is governed by an MIT-style
-- license that can be found in the LICENSE file.

-- Runs once after all inializations have ran but before the main loop.

material1 = material_test_new{x=500, y=500, effect=3, selection=0}

material2 = material_test_new{x=750, y=500, effect=2, selection=0}

material3 = material_test_new{x=1000, y=500, effect=1, selection=0}

material4 = material_test_new{x=1250, y=500, effect=4, selection=0}

dynamic_text = dynamic_text_test_new{x=100,y=100}