#ifndef commonvars_h
#define commonvars_h

#include <stdint.h>
#include "defines.h"
#include "Platform.h"

extern uint16_t ColorWhite, ColorBlack;
extern uint8_t gameState, debugMode, needRedraw;
extern bool EnemyStates[3][3];
extern bool PlayerStates[3];
extern uint8_t HitPosition, LivesLost;
extern uint32_t Teller, FlashesDelay, Flashes, Delay;
extern bool CanMove, CrashSoundPlayed;
extern uint32_t frames;
extern uint32_t Score;
extern uint8_t buttons,prevbuttons;
extern const uint8_t* imgBackground, *imgBigFont, *imgLcdFont, *imgEnemy, *imgPlayer;
#endif
