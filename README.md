# Bloodstream

Bloodstream is a game, built as a demonstration of techniques for a TTGO T-Display. It was originally a submission for a class, thus has some aspects that are to aid marking, as opposed to make a playable game.

It is somewhat inspired by the nerius from the Archer Season 6 Finale (without the explosion at the end...), and simulates a ship flying through a blood vessel to avoid enemies.
The enemies are at present, green rectangles, which are not particularly scary however I had planned to introduce a generic sprite rendering method, but I ran out of time (a throw back to Shoot Em Up Construction Kit, that I played with for hours as a child on Commodore64).

Features:
- Procedurally generated blood vessel walls
- Thrust is simulated with acceleration and deceleration for left and right movement

The game itself is simple, avoid the green rectangles as the speed increases. I have artificially increased the speed of levels (as a demo) so this will appear strange.

To run, you will need to have [PIO ](https://platformio.org/)

To run this on an emulator (assuming you do not have a real board, if you do, you should be able to just upload it to that), you should be able to select env:emulator as the Platformio project environment. This is defined within this Platformio project.

It is written in C, and utilises a graphics library written by [Dr Martin Johnson](https://github.com/dzo). This package also includes an emulator for the [TTGO board](https://github.com/dzo/ttgo-tdisplay-emulator).


