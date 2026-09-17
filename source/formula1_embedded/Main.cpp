#include <stdio.h>
#include <inttypes.h>
#include <float.h>
#include <math.h>
#include <string.h>

#include "commonvars.h"
#include "intro.h"
#include "sound.h"
#include "game.h"
#include "savestate.h"
#include "helperfuncs.h"

//The program itself, Game_Setup and Game_Loop are called by the device's own source

const uint32_t timePerFrame =  1000000 / FRAMERATE;
static float frameRate = 0;
static uint32_t currentTime = 0, lastTime = 0, frameTime = 0;
static bool endFrame = true;
bool webAppStore = false;

static uint32_t getFreeRam() {
  return Platform_FreeHeap();
}

static uint32_t getFreeStack() {
	return Platform_FreeStack();
}

//lowest free heap seen since boot, sampled at the end of Game_Setup and of every frame.
//Something allocated and freed again within one frame does not show up here
static uint32_t lowestFreeRam = UINT32_MAX;

static void trackLowestFreeRam()
{
    uint32_t freeRam = getFreeRam();
    if (freeRam < lowestFreeRam)
        lowestFreeRam = freeRam;
}

//multi line text straight to the screen. tft.drawChar already matches what the
//framebuffer version did per character, a 6x8 cell with the background only
//painted when it differs from the text colour, so only the line breaks are
//handled here. Advances match the old code, 6 pixels per char and 9 per line
static void tftPrint(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size)
{
	int16_t cursorX = x;
	int16_t cursorY = y;
	if (!str)
		return;
#if LOVYANGFX
	//LovyanGFX's drawChar that takes the colours hands them to the font the other way
	//round, set them as the text colour instead. Its default font is the same 6x8 GLCD
	//font and a background equal to the text colour is left out here as well
	GFX.setTextColor(color, bg);
	GFX.setTextSize(size);
#endif
#if SCREENBUFFER == 0
	//Straight to the display every character would be a write transaction of its own, and
	//the chip select sits on the I/O expander: that is I2C traffic per character. One
	//transaction for the whole text instead. Into a buffer nothing is sent, so nothing to do
	SCREEN.startWrite();
#endif
	while (*str)
	{
		if (*str == '\n')
		{
			cursorY += 9 * size;
			cursorX = x;
			str++;
			continue;
		}
#if LOVYANGFX
		GFX.drawChar((uint8_t)*str, cursorX, cursorY);
#else
		GFX.drawChar(cursorX, cursorY, *str, color, bg, size);
#endif
		cursorX += 6 * size;
		str++;
	}
#if SCREENBUFFER == 0
	SCREEN.endWrite();
#endif
}


static void printDebugCpuRamLoad()
{
    if(debugMode || FORCEDEBUG)
    {
        //the text is only put together a few times a second: every frame it would cost the
        //formatting and the heap and stack readings for figures nobody can read that fast.
        //It is still drawn every frame, the game may have been drawn over it
        static char debuginfo[80] = "";
        static uint32_t lastUpdate = 0;
        uint32_t now = Platform_Micros();
        if ((debuginfo[0] == '\0') || (now - lastUpdate >= 250000))
        {
            lastUpdate = now;
            int fps_int = (int)frameRate;
            int fps_frac = (int)((frameRate - fps_int) * 100);
            //S is the least sketch stack that has been free since boot, out of 4096 bytes
            //L: is the lowest free heap since boot, in the same column as R: on the line above
            snprintf(debuginfo, sizeof(debuginfo), "F:%3d.%2d R:%3" PRIu32 " \nS:%4" PRIu32 "   L:%3" PRIu32 " ", fps_int, fps_frac, getFreeRam(), getFreeStack(), lowestFreeRam);
            //Platform_Log("%s\n", debuginfo);
        }
        tftPrint(0, 0, debuginfo, SCREEN.color565(255,255,255), SCREEN.color565(0,0,0), 1);
    }
}

void Game_Setup(void)
{
    //webAppStore is set in Platform_Init
    Platform_Init("Formula1 v1.1");
    if(!webAppStore)
    {
        Platform_Log("Free Ram at boot game: %6" PRIu32 "\n", getFreeRam());
        debugMode = 0;
        needRedraw = 1;
        initSound();
        initMusic();
        setSoundOn(true);
        setMusicOn(true);
        loadSaveState();
        preloadImages();
        //with a 1 bpp buffer, the colours its set and clear bits are shown in. The skin is
        //always black & white there
        Platform_SetBufferColors(ColorWhite, ColorBlack);
        gameState = gsInitIntro;
        trackLowestFreeRam();
        currentTime = Platform_Micros();
        lastTime = 0;
    }
    else
    {
        //webappstore stuff
    }
}

void Game_Loop(void)
{
    if(!webAppStore)
    {        
        currentTime = Platform_Micros();
        frameTime  = currentTime - lastTime;
    #if FPSLOCK
        if((frameTime < timePerFrame) || !endFrame)
           return;
    #else
        //no lock, a frame starts as soon as the last one is done
        if(!endFrame)
           return;
    #endif
        endFrame = false;
        //without the lock two frames can start within the same microsecond on a fast PC
        frameRate = 1000000.0 / (frameTime ? frameTime : 1);
        lastTime = currentTime;
        processSound();
        prevbuttons = buttons;
        buttons = Platform_GetButtons();

        //debug mode is toggled with (A) and left and down together, whichever of them is
        //pressed last. The dpad alone would trigger it while playing
        const uint8_t debugCombo = BUTTON_A | BUTTON_LEFT | BUTTON_DOWN;
        if(((buttons & debugCombo) == debugCombo) && ((prevbuttons & debugCombo) != debugCombo))
        {
            debugMode = !debugMode;
            //the screens only draw what changed, the debug header has to be drawn over
            needRedraw = 1;
        }

        //gamestate handling
        switch (gameState)
        {
            case gsInitIntro:
            case gsIntro:
                intro();
                break;
            case gsInitGame:
            case gsGame:
                game();
                break;
            case gsInitGameIntro:
            case gsGameIntro:
                gameIntro();
                break;
            case gsInitGameOver:
            case gsGameOver:
                gameOver();
                break;
            default:
                break;
        }

        trackLowestFreeRam();
        printDebugCpuRamLoad();
        Platform_PresentFrame();
        endFrame = true;
    }
    else
    {
        //webappstore stuff
        static uint32_t prev = 0;
        if(Platform_Micros() - prev > 1000000)
        {
            prev = Platform_Micros();
            Platform_Log("Free Ram webappstore: %6" PRIu32 "\n", getFreeRam());
        }
    }
}
