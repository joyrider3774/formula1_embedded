#ifndef FOURBITIMAGE_H
#define FOURBITIMAGE_H

#include <stdint.h>
#include "Platform.h"

//Pictures four bits a pixel with a palette of their own, written by tools/fourbit.py. The layout is
//set out there; what matters here is that the header is 8 bytes, the palette follows it as
//OneBitAt-style little endian RGB565, and the pixels come after that two to a byte with the left
//one in the high nibble. A row always starts on a byte, so the row a tile begins at is reached by
//multiplying rather than by decoding what came before it, which is what a font sheet needs

#define FOURBIT_MAGIC 0x34
#define FOURBIT_HEADER 8
#define FOURBIT_FLAG_TRANSPARENT 0x01
//the pixels are run length encoded. Only a picture drawn whole is packed that way: a packed one can
//only be read from its first byte, so a sheet and anything drawn a part at a time is left as it is
#define FOURBIT_FLAG_RLE 0x02

//what the header holds. Inline for the same reason OneBitWidth is: a draw asks for these before it
//starts, so as calls in another file they cost thousands of them a frame to read a byte
static inline int FourBitFlags(const uint8_t* d) { return PLATFORM_READ_BYTE(d + 1); }
static inline int FourBitWidth(const uint8_t* d) { return PLATFORM_READ_BYTE(d + 2) | (PLATFORM_READ_BYTE(d + 3) << 8); }
static inline int FourBitHeight(const uint8_t* d) { return PLATFORM_READ_BYTE(d + 4) | (PLATFORM_READ_BYTE(d + 5) << 8); }
static inline int FourBitColours(const uint8_t* d) { return PLATFORM_READ_BYTE(d + 6); }
//where the pixels start, past the header and the palette
static inline int FourBitPixelsAt(const uint8_t* d) { return FOURBIT_HEADER + FourBitColours(d) * 2; }
//a row is this many bytes, two pixels to each
static inline int FourBitStride(const uint8_t* d) { return (FourBitWidth(d) + 1) / 2; }

//Draws the w by h part at sx,sy of a four bit picture at x,y on the screen, clipped to it. The
//picture carries its own width, so there is no dataWidth to pass
//With transparent set the pixels whose palette entry is 0 are skipped, which is what index 0 is
//reserved for when FOURBIT_FLAG_TRANSPARENT is set. A picture that carries no transparent colour
//is drawn whole whatever the call asks for
PLATFORM_FAST_CODE PLATFORM_HOT_CODE void drawImage4BitPart(int x, int y, int sx, int sy,
                                                            int w, int h, const uint8_t* data,
                                                            bool transparent);

#endif
