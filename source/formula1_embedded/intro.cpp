#include <stdint.h>
#include "commonvars.h"
#include "printfuncs.h"
#include "helperfuncs.h"
#include "intro.h"

#define FRAMEDELAY 60


int ay;
//the part of the intro on screen: the name, "presents" or nothing
static uint8_t shownStep;

void initIntro(void)
{
    frames = 0;
    ay = WINDOW_HEIGHT;
    needRedraw = 1;
}

void intro(void)
{
    if (gameState == gsInitIntro)
    {
        initIntro();
        gameState -= gsInitDiff;
    }

    frames++;
    uint8_t step = 2;
    if (frames < FRAMEDELAY)
        step = 0;
    else if (frames < FRAMEDELAY * 2)
        step = 1;

    //the screen is only drawn again when it shows something else, straight to the display it
    //would flicker otherwise
    if (needRedraw || (step != shownStep))
    {
        needRedraw = 0;
        shownStep = step;
        fillScreen(ColorBlack);
        if (step == 0)
        {
            printMessage(imgBigFont, (10-6) >> 1, 4, bigFontSize, "WILLEMS");
            printMessage(imgBigFont, (10-4) >> 1, 6, bigFontSize, " DAVY");
        }
        else if (step == 1)
        {
            printMessage(imgBigFont, (10-8)>>1, 5, bigFontSize, "PRESENTS");
        }
    }

    if (step == 2)
    {
        if(ay > 0)
        {
            ay -= 10;
        }
        else
        {
            gameState = gsInitGameIntro;
        }
    }

    if (buttons)
    {
        gameState = gsInitGameIntro;
    }
}
