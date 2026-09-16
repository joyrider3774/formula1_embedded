#ifndef SAVESTATE_H
#define SAVESTATE_H

#include <stdint.h>
void saveSaveState(void);
void loadSaveState(void);
void setHiScore(int value);
uint32_t getHiScore(void);

#endif