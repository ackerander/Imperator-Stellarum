# Imperator Stellarum

Space game with realistic gravity simulation. Successor of Astro.

## Dependencies

- libsdl2 (SDL2 libraries)
- sdl2_gfx

## Quick Start

```console
$ make
$ ./game
```

## How to play

When you start the game, it may seem to be frozen.
Do not panic!
This is because the celestial bodies are being simulated in real-time
(Space is so vast that even though they are moving at a blistering pace, compared to the vast empty space, they are hardly moving).

To step up the simulation speed, mouse over the time slider in the bottom right and scroll up (to speed up) or down (to slow down).
To make the planets/bodies more visible, there are three options:

* Zoom in on the planet/body using the scroll wheel (WASD pans the camera)
* Mouse over the inflation slider (bottom right) and scroll up (this makes the bodies appear larger and destroys the scale of the simulation).
* Click on the planet/body from the list and an indicator will pop up pointing to it.
	- If you middle click on the planet/body in the list, the camera will lock centered on it. Middle click somewhere else to unlock camera.
