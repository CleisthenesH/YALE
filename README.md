# Introduction
The scope of this project is the creation of a GUI widget engine in pure C allegro that can be scripted in LUA.
This is the accumulation of several smaller projects and is planned to be the starting point for later projects.

The core of this engine is a stack of widgets with the dual callbacks to update themselves and global state.

This will be my first public project in a long time and will also serve as a my reeducation in git, licencing, and making regular commits.
So please be pacience with the mess and any "works on my machine" responces.

# Scope 
The scope of this project is best meet by building the engine then test it by implementing a basic card game.
The game is meant to be super generic with no thought to ballancing and is only meant to test the workability of the engine.

# Objects Overview
The best overview of the engine is an overview of the objects in the engine.

## Widgets 
Widgets are the core of the engine.

They are a collection of data meant to provide basic functionally to listen to userevents, update, and draw to the screen.
Along with basic interaction like being dragable and callbacks when ciertain 
an object on screen and provide callbacks when the widget is interacted with.
Example callbacks include

### Pieces and Zones
Pieces and Zones are types of widgets that are grouped with a Piece Manager to manage a "move" process.
A move is a three step process of simply moving a piece from one zone to another.
1. Premove : 
	1. A piece is hovered triggering a manager callback to check which zones the piece can move to.
	2. The list is stored and for each zone callback is called to start a "highlight" mode.
2. Move :
	1. The piece is dragged.
	2. All zones on the list are set to draggable, all other zones are set no nonstackable.
3. Postmove :
	1. The zones' and pieces' state is changes such that piece is in the zone.
	2. A zone callback is called to stop the highlight mode of potential zones.
	3. A manager callback to inform what moves happened.


## Style Element
Meant to provide a simple interface to more complex rendering options.
It consists of three public components and one internal sub class.

### Tweener
The Tweener is the internal sub class and is basically a way to get smooth effects.
Each tweener has n channels

### Keyframe

### Materials

#### Selector
#### Effect

### Particles


