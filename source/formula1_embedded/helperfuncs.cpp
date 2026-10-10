#include <stdint.h>
#include "commonvars.h"
#include "helperfuncs.h"
//the art read from a card, where a card build gets its rows from
#include "cardimages.h"
#include "fourbitimage.h"
//the one bit pictures of the black & white skin, shared with anything else
//that reads them
#include "onebitimage.h"

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

//A card build has no art in flash at all: the pictures come off the card while the game runs,
//every skin of them, see cardimages.h. So none of the generated headers are part of it
#if !CARDIMAGES
//only the skin FORCESKIN picks is part of the build (a 1 bpp buffer forces the black & white one)
#if FORCESKIN == skinDefault
#include "images/default/background_RGB565_LE.h"
#include "images/default/bigfont_RGB565_LE.h"
#include "images/default/enemy_RGB565_LE.h"
#include "images/default/lcdfont_RGB565_LE.h"
#include "images/default/player_RGB565_LE.h"
#endif

#if FORCESKIN == skinDefault4b
#include "images/default_4b/background_RGB565_LE.h"
#include "images/default_4b/bigfont_RGB565_LE.h"
#include "images/default_4b/enemy_RGB565_LE.h"
#include "images/default_4b/lcdfont_RGB565_LE.h"
#include "images/default_4b/player_RGB565_LE.h"
#endif

#if FORCESKIN == skinBlackWhite
#include "images/black_white/background_RGB565_LE.h"
#include "images/black_white/bigfont_RGB565_LE.h"
#include "images/black_white/enemy_RGB565_LE.h"
#include "images/black_white/lcdfont_RGB565_LE.h"
#include "images/black_white/player_RGB565_LE.h"


#endif
#endif

//The skin in use. A flash build has the one FORCESKIN put in it and no other; a card build has
//every skin on the card and this is whichever of them is being shown, see CardImages_UseSkin
uint8_t currentSkin(void)
{
#if CARDIMAGES
    return CardImages_Skin();
#else
    return FORCESKIN;
#endif
}

void preloadImages(void)
{
#if CARDIMAGES
    //Every picture comes off the card, by the numbers tools/mkcard.py generated into cardindex.h.
    //The skin is not decided here any more: the card holds all of them and CardImages_UseSkin
    //picks one, so this runs again when that changes
    ColorWhite = SCREEN.color565(255,255,255);
    ColorBlack = SCREEN.color565(0,0,0);
    imgBackground = CardImages_Get(CARD_IMG_BACKGROUND);
    imgBigFont = CardImages_Get(CARD_IMG_BIGFONT);
    imgLcdFont = CardImages_Get(CARD_IMG_LCDFONT);
    imgEnemy = CardImages_Get(CARD_IMG_ENEMY);
    imgPlayer = CardImages_Get(CARD_IMG_PLAYER);
#else
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
#if FORCESKIN == skinDefault4b
        case skinDefault4b:
            ColorWhite = SCREEN.color565(255,255,255);
            ColorBlack = SCREEN.color565(0,0,0);
            imgBackground = default_4b_background_data;
            imgBigFont = default_4b_bigfont_data;
            imgLcdFont = default_4b_lcdfont_data;
            imgEnemy = default_4b_enemy_data;
            imgPlayer = default_4b_player_data;
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
#endif
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
#if ONEBITIMAGES
    //the skin's pictures carry their own width, the one passed in is the RGB565 path's
    (void)dataWidth;
    drawImageOneBitPart(x, y, sx, sy, w, h, data, false);
#elif FOURBITIMAGES
    //the same: a four bit picture carries its own width and its palette
    (void)dataWidth;
    drawImage4BitPart(x, y, sx, sy, w, h, data, false);
#else
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
#if CARDIMAGES
        //no flash array to index: the row comes from the arena or off the card, see cardimages.h
        if (!CardImages_Row(data, sx + c0, sy + r, cols, row))
            memset(row, 0, (size_t)cols * sizeof(uint16_t));
#else
        PLATFORM_READ_BYTES((uint8_t*)row, data + ((sy + r) * dataWidth + sx + c0) * sizeof(uint16_t), cols * sizeof(uint16_t));
#endif
  #if SCREENBUFFER == 16
        uint16_t* d = &((uint16_t*)buffer)[dy * WINDOW_WIDTH + dx];
        //a 16 bpp sprite keeps its pixels byte swapped
        for (int c = 0; c < cols; c++)
            d[c] = (uint16_t)((row[c] >> 8) | (row[c] << 8));
  #elif SCREENBUFFER == 8
        uint8_t* d = &((uint8_t*)buffer)[dy * WINDOW_WIDTH + dx];
        //RGB332, the same conversion SetBufferPixel does
        for (int c = 0; c < cols; c++)
            d[c] = ToBuffer332(row[c], (int16_t)(dx + c), (int16_t)dy);
  #else
        for (int c = 0; c < cols; c++)
            SetBufferBit((uint8_t*)buffer, dx + c, dy, row[c]);
  #endif
    }
#else
#if CARDIMAGES
    //THE BUS RULE: where the card shares the display's bus, reading it takes the bus over - the
    //panel is deselected and the SPI set up for the card, see Platform_CardRead. So a row is
    //fetched with nothing of the display's open and only then sent, each row in a transaction of
    //its own. A window around the whole part cannot survive a card read in the middle of it
    for (int r = r0; r < r1; r++)
    {
        if (!CardImages_Row(data, sx + c0, sy + r, cols, row))
            memset(row, 0, (size_t)cols * sizeof(uint16_t));
        SCREEN.startWrite();
  #if LOVYANGFX
        SCREEN.setAddrWindow(dx, y + r, cols, 1);
        //true: the values are plain RGB565, the library puts them in display order
        SCREEN.writePixels(row, cols, true);
  #else
        GFX.pushImage(dx, y + r, cols, 1, row);
  #endif
        SCREEN.endWrite();
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
#endif
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
