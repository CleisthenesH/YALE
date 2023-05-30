**:warning:Under Construction:warning:**
# :goat: YALE :goat:

The Yale is a mythical goat like creature with an additional pair of tusks and horns that can swivel around.

<p align="center">
  <img src="https://upload.wikimedia.org/wikipedia/commons/6/6d/Yale_salient.gif" alt="A drawing of a yale">
  <a href="https://en.wikipedia.org/wiki/Yale_(mythical_creature)">
    <br>
    A depiction of a yale from wikimedia commons. Note the backwards horn.
  </a>
</p>

But YALE also stands for "Yet Another Lua Engine", a succinct description of this project


# Introduction
The goal of this project is to create GUI widget engine in pure-C [Allegro](https://liballeg.org/) that is scriptable by [Lua](https://www.lua.org/).
And the core of this project is the widget interface stack with a system of inbuilt callbacks based on user generated events.

The easiest way to show this would be showing the workflow for creating a button, setting it's location and text, and making it print to the console when clicked. 

## Lua Code
```lua
button = button_new{x=500, y=500, text="Click Me!"}

function button:left_click()
	print(self, " was clicked!")
end
```
## Rendered Button Pre-click
![Image of the above code pre-click.](/GUI_Widgets/docs/README/Pre-click.png)

## Rendered Button Post-click
![Image of the above code post-click.](/GUI_Widgets/docs/README/Post-click.png)

## Excerpt of Engine Code for Rendering the Button
```c
struct button
{
	struct widget_interface* widget_interface;
	char* text;
	ALLEGRO_FONT* font;
	ALLEGRO_COLOR color;
};

static void draw(const struct widget_interface* const widget)
{
	const struct button* const button = (struct button*) widget->upcast;
	const double half_width = button->widget_interface->render_interface->half_width;
	const double half_height = button->widget_interface->render_interface->half_height;

	al_draw_filled_rounded_rectangle(-half_width, -half_height, half_width, half_height, 10, 10, button->color);

	if (button->text)
		al_draw_text(button->font, al_map_rgb_f(1, 1, 1),
			0, -0.5 * al_get_font_line_height(button->font),
			ALLEGRO_ALIGN_CENTRE, button->text);

	al_draw_rounded_rectangle(-half_width, -half_height, half_width, half_height, 10, 10, al_map_rgb(0, 0, 0), 2);
}
```
# Why use YALE instead of other Lua game engine
As suggested by the repository title, I'm well aware that a lot of game engines with a Lua interface exist.
So a big question is why use YALE instead of other engines.

For those familiar with Lua the obvious comparison is the [LÖVE](https://github.com/love2d/love),
being feature rich and well documented project with an active community and a history of releases going back decades.
And the YALE engine is all the better for existing in an ecosystem with such high quality projects as LÖVE.

The three benefits YALE has over LÖVE are:
1. In YALE provides a clean interface for programing widgets in ALLEGRO-C with only game logic in Lua. Making YALE more approachable for those coming from a pure C / ALLEGRO background.
2. YALE's only dependencies are ALLEGRO, and by extension OPEN-GL, and Lua making the dependency surface a smaller.
3. YALE has a bunch of widget and board management logic pre-defined.

The benefits LÖVE over YALE are:
1. Literally everything else.

So while LÖVE is the clear choice for most application I think YALE has some niche use case with more focused vision of writing widget and board management in ALLEGRO-C.

# Scope 
The scope of this project is best meet by building the engine then test it by implementing a basic card game.
The game is meant to be super generic with no thought to balancing and is only meant to test the workability of the engine.

# Roles
This project is designed with three roles in mind.
Each role has responsibilities to the other roles when writing features and a set of expected skills.

## Engine maintainer
The engine maintainer is the broadest role and has access to raw memory, work scheduling, and orchestrating callbacks.
The engine maintainer makes interfaces for the other roles to easily define widgets without worrying about engine implementation

## Widget maker
The Widget maker takes the engine takes the engine maintainer's interfaces and makes widgets for the widget user.	
It is the responsibility of the widget maker that the widget user should be able to manipulate the widget as they please while staying within Lua.

## Widget user
The Widget user only works in Lua and doesn't understand how widgets are implemented but uses them to interface with domain knowledge.
For example the widget user can make a game by making a board from widgets and using callbacks to process game logic.

# Objects Overview
The best overview of the engine is an overview of the objects in the engine.

## Widgets 
Widgets are the core of the engine.

They are a collection of data meant to provide basic functionally to listen to userevents, update, and draw to the screen.
Along with basic interaction like being draggable and callbacks when certain 
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

## Renderer
The renderer system provides a simple interfaces to achieve complex effects and features in a intuitive workflow that is integrated with the widget engine.

### Tweener
The Tweener is the internal sub object that interoperates n channels between given timestamps.
It's role is to provide smooth and continuous values for making more complex features.
The memory is quite raw and should only be accessed by the engine maintainer role.

### Particles
Particles are simple objects that have three self-explanatory methods:
Update, Draw, and Garbage Collection.

They are lightweight and have an erratic lifetime.
Because of this they are placed in particles buckets when created and instead of interacting with a single particle that may or may not exist you interact with the bucket instead.

### Materials
Before normal rendering functions are called a material can be applied.	
These materials interface with the shader and can add spectral or procedural effects to comparatively flat renders.

#### Selector
Which parts of the drawing operation should deviate from standard rendering procedure.

#### Effect
How the rendering should deviate from standard rendering procedure.

### Renderer Interface
Meant to provide a simple interface to more complex rendering options.
It consists of three public components and one internal sub class.

#### Keyframes
Keyframes are based on the parameters required to call al_build_transform: Position, Rotation, and Scale.
They are used to build a transform before the render interfaces calls it's draw function.
Enabling the manipulation of the widget around the screen while requiring the widget writer to only work in local coordinates.

#### Effect
There are some planned global effects to be applied when the interface's draw function is called.
Planned include saturating the color to white or burning the object to nothing.

# Acknowledgments
## Code
Lua (5.4) [Website](https://www.lua.org/)

Allegro (5.2) [Website](https://liballeg.org/)
## Art
Emily Huo (Font) [itch.io](https://emhuo.itch.io/)

Corax Digital Art (Character Art) [Link Tree](https://linktr.ee/coraxdigitalart)

Game-Icons (Icons) [Website](https://game-icons.net)

Icons by:
- Lorc, http://lorcblog.blogspot.com
- Delapouite, https://delapouite.com
- John Colburn, http://ninmunanmu.com
- Felbrigg, http://blackdogofdoom.blogspot.co.uk
- John Redman, http://www.uniquedicetowers.com
- Carl Olsen, https://twitter.com/unstoppableCarl
- Sbed, http://opengameart.org/content/95-game-icons
- PriorBlue
- Willdabeast, http://wjbstories.blogspot.com
- Viscious Speed, http://viscious-speed.deviantart.com - CC0
- Lord Berandas, http://berandas.deviantart.com
- Irongamer, http://ecesisllc.wix.com/home
- HeavenlyDog, http://www.gnomosygoblins.blogspot.com
- Lucas
- Faithtoken, http://fungustoken.deviantart.com
- Skoll
- Andy Meneely, http://www.se.rit.edu/~andy/
- Cathelineau
- Kier Heyl
- Aussiesim
- Sparker, http://citizenparker.com
- Zeromancer - CC0
- Rihlsul
- Quoting
- Guard13007, https://guard13007.com
- DarkZaitzev, http://darkzaitzev.deviantart.com
- SpencerDub
- GeneralAce135
- Zajkonur
- Catsu
- Starseeker
- Pepijn Poolman
- Pierre Leducq
- Caro Asercion

