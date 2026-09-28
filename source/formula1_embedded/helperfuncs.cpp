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

#if ONEBITIMAGES
// ===========================================================================
// One bit images
//
// The black & white skin is packed one bit a pixel rather than kept as RGB565, see tools/onebit.py
// for the format. A game built with that skin has none of the RGB565 paths below in it, so the
// flash a second decoder would take is not spent.
//
// It is quicker as well as smaller. A row of a picture is packed the very same way the 1 bpp
// screen buffer keeps its own row, so where the two are in step a row is a memcpy and nothing at
// all is worked out per pixel, which is what a full screen background does.
// ===========================================================================

#define ONEBIT_HEADER 8
//what a set and a clear bit stand for, which is what the skin was drawn in
#define ONEBIT_SET 0xFFFF
#define ONEBIT_CLEAR 0x0000
//no picture is wider than the screen, so no row of one is either
#define ONEBIT_MAX_STRIDE ((WINDOW_WIDTH + 7) / 8)

static inline int OneBitWidth(const uint8_t* d) { return PLATFORM_READ_BYTE(d + 2) | (PLATFORM_READ_BYTE(d + 3) << 8); }
static inline int OneBitHeight(const uint8_t* d) { return PLATFORM_READ_BYTE(d + 4) | (PLATFORM_READ_BYTE(d + 5) << 8); }
static inline int OneBitMaskAt(const uint8_t* d) { return PLATFORM_READ_BYTE(d + 6) | (PLATFORM_READ_BYTE(d + 7) << 8); }
//reads the bit of column c out of an unpacked row
static inline bool OneBitAt(const uint8_t* row, int c) { return (row[c >> 3] & (0x80 >> (c & 7))) != 0; }

//unpacks one row of a plane and says where the next one starts
PLATFORM_FAST_CODE static const uint8_t* OneBitRow(const uint8_t* p, uint8_t* row, int stride)
{
    int done = 0;
    while (done < stride)
    {
        const uint8_t control = PLATFORM_READ_BYTE(p++);
        int n = (control & 0x7F) + 1;
        if (n > stride - done)
            n = stride - done;
        if (control & 0x80)
        {
            memset(row + done, PLATFORM_READ_BYTE(p), n);
            p++;
        }
        else
        {
            PLATFORM_READ_BYTES(row + done, p, n);
            p += n;
        }
        done += n;
    }
    return p;
}

//passes over whole rows without unpacking them, for the rows above the part being drawn. Every
//row is encoded on its own, which is what makes this possible at all
PLATFORM_FAST_CODE static const uint8_t* OneBitSkip(const uint8_t* p, int rows, int stride)
{
    while (rows-- > 0)
    {
        int done = 0;
        while (done < stride)
        {
            const uint8_t control = PLATFORM_READ_BYTE(p++);
            const int n = (control & 0x7F) + 1;
            p += (control & 0x80) ? 1 : n;
            done += n;
        }
    }
    return p;
}

//Draws the w x h part at sx,sy of a one bit image at x,y on the screen, clipped to it. With
//transparent set the pixels its mask clears are skipped, and a picture that has nothing to skip
//carries no mask at all
PLATFORM_FAST_CODE void drawImageOneBitPart(int x, int y, int sx, int sy, int w, int h,
                                            const uint8_t* data, bool transparent)
{
    if (!data || (w <= 0) || (h <= 0))
        return;
    const int dataWidth = OneBitWidth(data);
    const int dataHeight = OneBitHeight(data);
    const int maskAt = OneBitMaskAt(data);
    const bool useMask = transparent && (maskAt != 0);

    //where the image's top left corner lands, and what of it is drawn: the same clipping the
    //RGB565 path does
    const int ox = x - sx;
    const int oy = y - sy;
    int c0 = sx, c1 = sx + w, r0 = sy, r1 = sy + h;
    if (c0 < 0) c0 = 0;
    if (r0 < 0) r0 = 0;
    if (c1 > dataWidth) c1 = dataWidth;
    if (r1 > dataHeight) r1 = dataHeight;
    if (c0 < -ox) c0 = -ox;
    if (r0 < -oy) r0 = -oy;
    if (c1 > WINDOW_WIDTH - ox) c1 = WINDOW_WIDTH - ox;
    if (r1 > WINDOW_HEIGHT - oy) r1 = WINDOW_HEIGHT - oy;
    if ((c0 >= c1) || (r0 >= r1))
        return;

    const int stride = (dataWidth + 7) / 8;
    const uint8_t* pixels = OneBitSkip(data + ONEBIT_HEADER, r0, stride);
    const uint8_t* mask = useMask ? OneBitSkip(data + maskAt, r0, stride) : NULL;
    uint8_t rowPixels[ONEBIT_MAX_STRIDE];
    uint8_t rowMask[ONEBIT_MAX_STRIDE];

#if SCREENBUFFER == 1
    uint8_t* dst = (uint8_t*)SCREENBUFFER_PIXELS();
    if (!dst)
        return;
    //the buffer's rows start on a byte boundary, the screen being a whole number of bytes wide
    const int dstStride = WINDOW_WIDTH / 8;
    //true when a byte of the picture is a byte of the buffer, so the two can be copied
    const bool aligned = ((((ox + c0) ^ c0) & 7) == 0) && ((c0 & 7) == 0);
    for (int cy = r0; cy < r1; cy++)
    {
        pixels = OneBitRow(pixels, rowPixels, stride);
        if (mask)
            mask = OneBitRow(mask, rowMask, stride);
        uint8_t* dstRow = dst + (oy + cy) * dstStride;
        int c = c0;
        //the whole bytes in the middle are the picture's own. A full screen background is one
        //memcpy a row and nothing else
        if (!mask && aligned)
        {
            const int whole = (c1 - c0) >> 3;
            if (whole > 0)
            {
                memcpy(dstRow + ((ox + c0) >> 3), rowPixels + (c0 >> 3), whole);
                c = c0 + (whole << 3);
            }
        }
        for (; c < c1; c++)
        {
            if (mask && !OneBitAt(rowMask, c))
                continue;
            const int dx = ox + c;
            const uint8_t bit = 0x80 >> (dx & 7);
            if (OneBitAt(rowPixels, c))
                dstRow[dx >> 3] |= bit;
            else
                dstRow[dx >> 3] &= (uint8_t)~bit;
        }
    }
#else
  #if SCREENBUFFER
    void* dst = SCREENBUFFER_PIXELS();
    if (!dst)
        return;
  #else
    //one transaction for the whole part, and one window when nothing is skipped
    uint16_t line[WINDOW_WIDTH];
    SCREEN.startWrite();
    if (!useMask)
        SCREEN.setAddrWindow(ox + c0, oy + r0, c1 - c0, r1 - r0);
  #endif
    for (int cy = r0; cy < r1; cy++)
    {
        pixels = OneBitRow(pixels, rowPixels, stride);
        if (mask)
            mask = OneBitRow(mask, rowMask, stride);
  #if SCREENBUFFER
        for (int c = c0; c < c1; c++)
        {
            if (mask && !OneBitAt(rowMask, c))
                continue;
            SetBufferPixel(dst, (int16_t)(ox + c), (int16_t)(oy + cy),
                           OneBitAt(rowPixels, c) ? ONEBIT_SET : ONEBIT_CLEAR);
        }
  #else
        if (!mask)
        {
            for (int c = c0; c < c1; c++)
                line[c - c0] = OneBitAt(rowPixels, c) ? ONEBIT_SET : ONEBIT_CLEAR;
    #if LOVYANGFX
            SCREEN.writePixels(line, c1 - c0, true);
    #else
            SCREEN.pushPixels(line, c1 - c0);
    #endif
        }
        else
        {
            //every run of pixels that is not skipped goes out in a window of its own
            int c = c0;
            while (c < c1)
            {
                while ((c < c1) && !OneBitAt(rowMask, c))
                    c++;
                const int start = c;
                while ((c < c1) && OneBitAt(rowMask, c))
                {
                    line[c - start] = OneBitAt(rowPixels, c) ? ONEBIT_SET : ONEBIT_CLEAR;
                    c++;
                }
                if (c == start)
                    continue;
                SCREEN.setAddrWindow(ox + start, oy + cy, c - start, 1);
    #if LOVYANGFX
                SCREEN.writePixels(line, c - start, true);
    #else
                SCREEN.pushPixels(line, c - start);
    #endif
            }
        }
  #endif
    }
  #if !SCREENBUFFER
    SCREEN.endWrite();
  #endif
#endif
}
#endif

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
            d[c] = ToBuffer332(row[c], (int16_t)(dx + c), (int16_t)dy);
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
