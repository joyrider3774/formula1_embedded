#ifndef helperfuncs_h
#define helperfuncs_h

#include <stdint.h>

void preloadImages(void);
uint8_t currentSkin(void);
void fillScreen(uint16_t color);
void drawImage(int x, int y, int w, int h, const uint8_t* data);
void drawImagePart(int x, int y, int sx, int sy, int w, int h, const uint8_t* data, int dataWidth);
void drawBackground(int x, int y, int w, int h);
void drawTile(const uint8_t* tiles, int tileSize, int tile, int x, int y);
#endif
