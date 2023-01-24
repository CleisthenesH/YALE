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
A sub type of widget meant to manage 

## Style Element
Meant to provide a simple interface to more complex rendering options.
It consists of three public components and one internal sub class.

### Tweener
The Tweener is the internal sub class and is basically a way to get smooth effects.

### Keyframe

### Materials

#### Selector
#### Effect

### Particles


