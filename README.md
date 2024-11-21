# Bloodstream

Bloodstream is a game, built as a demonstration of techniques for a TTGO T-Display. It was originally a submission for a class, thus has some aspects that are to aid marking. There was no skeleton provided, but we could utilise a graphics abstraction library that was provided (and is included as a dependency) that simplifies writing to the TTGO frame buffer.

It is somewhat inspired by the nerius from the Archer Season 6 Finale (without the explosion at the end...), and simulates a ship flying through a blood vessel.
The enemies are at present, green rectangles, which are not particularly scary, and I have future plans to introduce a sprite rendering abstraction (a throw back to Shoot Em Up Construction Kit, that I played with for hours as a child on Commodore64).

Features:
- Procedurally generated graphics (blood vessel walls, bubbles in menu screen)
- Simulated thrust and friction physics for left and right movement

The game itself is simple, avoid the green rectangles as the speed increases. I have artificially increased the speed of level incrementing to demonstrate that this worked, so this happens far more rapidly than an actual game would.

To run, you will need to have [PIO ](https://platformio.org/). The Platformio build information should take care of all the other dependencies for you.

To run this on an emulator (assuming you do not have a real board, if you do, you should be able to just upload it to that), you should be able to select env:emulator as the Platformio project environment. This is defined within this Platformio project and will download when you attempt to build. The emulator only currently works on x86-64 systems as far as I am aware.

It is written in C, and utilises a graphics library and emulator from [Dr Martin Johnson](https://github.com/dzo). This package also includes an emulator for the [TTGO board](https://github.com/dzo/ttgo-tdisplay-emulator).


