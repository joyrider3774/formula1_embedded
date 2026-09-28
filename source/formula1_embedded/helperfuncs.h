#ifndef helperfuncs_h
#define helperfuncs_h

#include <stdint.h>
//for PLATFORM_FAST_CODE, which marks the calls that run once for every pixel
#include "Platform.h"

void preloadImages(void);
uint8_t currentSkin(void);
void fillScreen(uint16_t color);
PLATFORM_FAST_CODE void drawImage(int x, int y, int w, int h, const uint8_t* data);
PLATFORM_FAST_CODE void drawImagePart(int x, int y, int sx, int sy, int w, int h, const uint8_t* data, int dataWidth);
PLATFORM_FAST_CODE void drawBackground(int x, int y, int w, int h);
PLATFORM_FAST_CODE void drawTile(const uint8_t* tiles, int tileSize, int tile, int x, int y);
#endif
