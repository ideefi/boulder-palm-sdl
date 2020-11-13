Boulder Palm on PC
Copyright (C) 2001-2019 by Wojciech Martusewicz <martusewicz@interia.pl>

Moving by arrow buttons or:
a - left
d - right
w - up
s - down

Moving preceded by a spacebar key results in an action in the given direction without actually moving.

Other keys:
q - quit
r - restart level
n - next level
p - previous level
m - mute (sound on/off)

Contribution:
============

It's open game! Contribution is very welcomed!
Everyone can create new levels, modify graphics or source code.
Please send me back results of your work (particularly new levels!).

Levels:
------
Levels are stored in *.lvl files in /res directory.
Level's size is now defined in source code (40x22 tiles), but can be freely changed:
    #define LEVELS_WIDTH    40
    #define LEVELS_HIGH     22

0 - TUNNEL (empty space)
1 - WALL (brick)
2 - HERO (should be one on the board)
3 - ROCK
4 - DIAMOND (treasure to pick up)
5 - GROUND (soil, can be digged)
6 - METAL (solid wall, withstand explosion)
7 - BOX (enemy)
8 - DOOR (should be one on the board)  
9 - FLY (enemy - change into diamonds)

.d - number of diamonds to pick up required to finish the level
.t - allowed time for level
Comments must be preceded by '#'.

Graphics:
--------
Bitmaps of tiles are stored in bmp files in /res directory
Now are in very poor quality and resolution.

You can create your own bitmaps, even completly different!
Resolution is not fixed, and bitmaps are scalable.

Dimensions of tiles displayed on the screen can by changed in source code (now 30 pixels):
    #define TILE_SIZE       30

Size of the game window can be changed as well (now 640x480):
    #define SCREEN_SIZE_X   640
    #define SCREEN_SIZE_Y   480

Source code:
-----------
The game is developed in ANSI C (C89), compiled against SDL2 library, and is very portable.
Originally created for PalmOS and J2ME. Now also available on Windows, Linux, Arduino, etc.
