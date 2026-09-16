#ifndef defines_h
#define defines_h

//the device comes first: the display library and SCREENBUFFER are device settings, see
//PlatformESPboy.h / PlatformSDL.h
#include "PlatformDevice.h"

//the ESPboy display
#define WINDOW_WIDTH 128
#define WINDOW_HEIGHT 128

#define gsGame 0
#define gsIntro 1
#define gsGameIntro 2
#define gsGameOver 3

#define gsInitDiff 50

#define gsInitGame (gsInitDiff + gsGame)
#define gsInitIntro (gsInitDiff + gsIntro)
#define gsInitGameIntro (gsInitDiff + gsGameIntro)
#define gsInitGameOver (gsInitDiff + gsGameOver)

//the lcd font's tiles are 8x8, the big font's 12x12
#define lcdFontSize 8
#define bigFontSize 12

//where the cars are drawn: a column of the road and a row of the enemies coming down it
#define carX(column) (22 + ((column) * 34))
#define enemyY(row) (10 + ((row) * 36))
#define playerY 104

#define skinDefault 0
#define skinBlackWhite 1

//FORCESKIN: -1 = the default skin, or the black & white one with a 1 bpp buffer, n = skin n
//(0 default, 1 black & white). There is no skin option in the game, so only the skin used is
//built in. A 1 bpp buffer can only show the black & white skin, so it forces that one.
//Set by the device header or the build
#if SCREENBUFFER == 1
  #if defined(FORCESKIN) && (FORCESKIN >= 0) && (FORCESKIN != skinBlackWhite)
  #error "a 1 bpp buffer can only show the black & white skin, FORCESKIN has to be -1 or 1"
  #endif
  #undef FORCESKIN
  #define FORCESKIN skinBlackWhite
#elif !defined(FORCESKIN) || (FORCESKIN < 0)
  #undef FORCESKIN
  #define FORCESKIN skinDefault
#endif

#define FRAMERATE 30
//1 = every frame waits until 1/FRAMERATE of a second has passed, 0 = a frame starts as soon
//as the last one is done, to see how fast the game can go. The game speed, the flashing
//and the music all count frames, so without the lock they run faster as well.
//A build can set it itself
#ifndef FPSLOCK
#define FPSLOCK 1
#endif
//1 = the debug header (frame rate, free heap and stack) is always shown, Up + Down does not
//hide it. 0 = it starts hidden and Up + Down shows and hides it. A build can set it itself
#ifndef FORCEDEBUG
#define FORCEDEBUG 0
#endif

#endif
