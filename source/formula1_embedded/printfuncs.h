#ifndef PRINTFUNCS_H
#define PRINTFUNCS_H

#include <stdint.h>

void printNumber(const uint8_t fontBitmapTable[], int32_t ax, int32_t ay, uint16_t tileSize, int32_t aNumber, int32_t maxDigits);
void printMessage(const uint8_t fontBitmapTable[], int32_t ax, int32_t ay, uint16_t tileSize, const char* amsg);



#endif