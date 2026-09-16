#include <stdint.h>
#include "commonvars.h"

uint8_t gameState, debugMode, needRedraw;
bool EnemyStates[3][3];
bool PlayerStates[3];
uint8_t HitPosition, LivesLost;
uint32_t Teller, FlashesDelay, Flashes, Delay;
uint32_t Score;
bool CanMove, CrashSoundPlayed;
uint32_t frames;
uint8_t buttons, prevbuttons = 0;
const uint8_t* imgBackground, *imgBigFont, *imgLcdFont, *imgEnemy, *imgPlayer;
uint16_t ColorWhite, ColorBlack;
