#include <stdint.h>
#include "commonvars.h"
#include "helperfuncs.h"

//A row of an image on its way to the display. Where flash is plain memory an evenly placed
//row is handed over where it lies, otherwise it is copied into the scratch row first. A 16
//bit read needs an even address, a core like the Cortex-M0+ faults on an odd one
static inline const uint16_t* ImageRow(const void* src, uint16_t* scratch, int count)
{
#if PLATFORM_DIRECT_FLASH
    if (((uintptr_t)src & 1) == 0)
        return (const uint16_t*)src;
#endif
    PLATFORM_READ_BYTES((uint8_t*)scratch, src, count * sizeof(uint16_t));
    return scratch;
}

//only the skin FORCESKIN picks is part of the build (a 1 bpp buffer forces the black & white one)
#if FORCESKIN == skinDefault
#include "images/default/background_RGB565_LE.h"
#include "images/default/bigfont_RGB565_LE.h"
#include "images/default/enemy_RGB565_LE.h"
#include "images/default/lcdfont_RGB565_LE.h"
#include "images/default/player_RGB565_LE.h"
#endif

#if FORCESKIN == skinBlackWhite
#include "images/black_white/background_RGB565_LE.h"
#include "images/black_white/bigfont_RGB565_LE.h"
#include "images/black_white/enemy_RGB565_LE.h"
#include "images/black_white/lcdfont_RGB565_LE.h"
#include "images/black_white/player_RGB565_LE.h"


#endif

//the skin in use, the one FORCESKIN builds in
uint8_t currentSkin(void)
{
    return FORCESKIN;
}

void preloadImages(void)
{
    switch(currentSkin())
    {
#if FORCESKIN == skinDefault
        case skinDefault:
            ColorWhite = SCREEN.color565(255,255,255);
            ColorBlack = SCREEN.color565(0,0,0);
            imgBackground = default_background_data;
            imgBigFont = default_bigfont_data;
            imgLcdFont = default_lcdfont_data;
            imgEnemy = default_enemy_data;
            imgPlayer = default_player_data;
            break;
#endif
#if FORCESKIN == skinBlackWhite
        case skinBlackWhite:
            ColorWhite = SCREEN.color565(255,255,255);
            ColorBlack = SCREEN.color565(0,0,0);
            imgBackground = black_white_background_data;
            imgBigFont = black_white_bigfont_data;
            imgLcdFont = black_white_lcdfont_data;
            imgEnemy = black_white_enemy_data;
            imgPlayer = black_white_player_data;
            break;
#endif
    }
}

void fillScreen(uint16_t color)
{
    GFX.fillRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, color);
}

//Draws the w x h part at sx,sy of a raw RGB565 little endian image that is dataWidth pixels
//wide, at x,y on the screen and clipped to it. Every visible row comes out of flash in one
//copy: LovyanGFX reads image data through plain pointers, but PROGMEM on the ESP8266 is flash
//that only takes 32 bit reads, so the rows are read here with PLATFORM_READ_BYTES
void drawImagePart(int x, int y, int sx, int sy, int w, int h, const uint8_t* data, int dataWidth)
{
    if (!data)
        return;
    //the columns and rows of the part that are on screen
    const int c0 = (x < 0) ? -x : 0;
    const int c1 = (x + w > WINDOW_WIDTH) ? WINDOW_WIDTH - x : w;
    const int r0 = (y < 0) ? -y : 0;
    const int r1 = (y + h > WINDOW_HEIGHT) ? WINDOW_HEIGHT - y : h;
    if ((c0 >= c1) || (r0 >= r1))
        return;
    const int cols = c1 - c0;
    const int dx = x + c0;
    //little endian RGB565 like every device, so the bytes can be copied straight into it
    uint16_t row[WINDOW_WIDTH];
#if SCREENBUFFER
    void* buffer = SCREENBUFFER_PIXELS();
    if (!buffer)
        return;
    for (int r = r0; r < r1; r++)
    {
        const int dy = y + r;
        PLATFORM_READ_BYTES((uint8_t*)row, data + ((sy + r) * dataWidth + sx + c0) * sizeof(uint16_t), cols * sizeof(uint16_t));
  #if SCREENBUFFER == 16
        uint16_t* d = &((uint16_t*)buffer)[dy * WINDOW_WIDTH + dx];
        //a 16 bpp sprite keeps its pixels byte swapped
        for (int c = 0; c < cols; c++)
            d[c] = (uint16_t)((row[c] >> 8) | (row[c] << 8));
  #elif SCREENBUFFER == 8
        uint8_t* d = &((uint8_t*)buffer)[dy * WINDOW_WIDTH + dx];
        //RGB332, the same conversion SetBufferPixel does
        for (int c = 0; c < cols; c++)
            d[c] = (uint8_t)(((row[c] & 0xE000) >> 8) | ((row[c] & 0x0700) >> 6) | ((row[c] & 0x0018) >> 3));
  #else
        for (int c = 0; c < cols; c++)
            SetBufferBit((uint8_t*)buffer, dx + c, dy, row[c]);
  #endif
    }
#else
    //straight to the display. The chip select sits on the I/O expander, every write
    //transaction costs I2C traffic, so all the rows go out in one
    SCREEN.startWrite();
  #if LOVYANGFX
    //one window for the whole part, filled a row at a time
    SCREEN.setAddrWindow(dx, y + r0, cols, r1 - r0);
  #endif
    for (int r = r0; r < r1; r++)
    {
        const uint16_t* prow = ImageRow(data + ((sy + r) * dataWidth + sx + c0) * sizeof(uint16_t), row, cols);
  #if LOVYANGFX
        //true: the values are plain RGB565, the library puts them in display order
        SCREEN.writePixels(prow, cols, true);
  #else
        GFX.pushImage(dx, y + r, cols, 1, prow);
  #endif
    }
    SCREEN.endWrite();
#endif
}

void drawImage(int x, int y, int w, int h, const uint8_t* data)
{
    drawImagePart(x, y, 0, 0, w, h, data, w);
}

//the background is as big as the screen, so the part at x,y is the part that belongs there
void drawBackground(int x, int y, int w, int h)
{
    drawImagePart(x, y, x, y, w, h, imgBackground, WINDOW_WIDTH);
}

//the fonts are a column of square tiles, tile 0 at the top
void drawTile(const uint8_t* tiles, int tileSize, int tile, int x, int y)
{
    drawImagePart(x, y, 0, tile * tileSize, tileSize, tileSize, tiles, tileSize);
}
