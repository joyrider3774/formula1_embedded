#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "commonvars.h"
#include "printfuncs.h"
#include "helperfuncs.h"

// function provided by @Pharap
// If the array is not big enough,
int32_t formatInteger(char *array, int32_t integer)
{
    int32_t maxDigits = 10;
    int32_t maxCharacters = (maxDigits + 1);
    int32_t lastIndex = (maxCharacters - 1);

    array[lastIndex] = '\0';

    if (integer == 0)
    {
        array[lastIndex - 1] = '0';
        return 1;
    }

    int32_t digits = 0;

    do
    {
        int32_t digit = integer % 10;
        integer /= 10;

        ++digits;

        array[lastIndex - digits] = ('0' + digit);
    } while (integer > 0);

    return digits;
};

//print a number on the game screen, right aligned in maxDigits tiles
void printNumber(const uint8_t fontBitmapTable[], int32_t ax, int32_t ay, uint16_t tileSize, int32_t aNumber, int32_t maxDigits)
{
    const int32_t buffSize = 10;
    char number[10 + 1]; //buffsize + 1
    int32_t digits = formatInteger(number, aNumber);
    int32_t maxFor = digits;
    if (digits > maxDigits)
        maxFor = maxDigits;
    for (int32_t c = 0; c < maxFor; c++)
    {
        if (number[buffSize - digits + c] == '\0')
            return;

        drawTile(fontBitmapTable, tileSize, number[buffSize - digits + c] - 47, (ax + (maxDigits - digits) + c) * tileSize, ay * tileSize);
    }
}

//print a message on ax,ay, the font tiles contain an alphabet
void printMessage(const uint8_t fontBitmapTable[], int32_t ax, int32_t ay, uint16_t tileSize, const char* amsg)
{
    // based on input from @Pharap
    const char *p = amsg;
    int32_t index = 0;

    while (1)
    {
        char fChar = *(p++);
        int tile = 0;
        switch (fChar)
        {
            case '\0':
                return;

            case ':':
                tile = 37;
                break;

            case 'a':
                tile = 40;
                break;

            case 'b':
                tile = 38;
                break;

            default:
                if ((fChar >= 'A') && (fChar <= 'Z'))
                    tile = fChar - 54;

                if ((fChar >= '0') && (fChar <= '9'))
                    tile = fChar - 47;
                break;
         }
        drawTile(fontBitmapTable, tileSize, tile, (ax + index) * tileSize, ay * tileSize);
        ++index;
    }
}
