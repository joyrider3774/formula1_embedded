#include <string.h>
#include <stdint.h>
#include "commonvars.h"
#include "savestate.h"

#define SAVE_MAGIC 0xDBDB

//the ESPboy version saved this at the start of its EEPROM, it is kept there so its high score
//is still read
#pragma pack(push, 1)
struct SaveData {
    uint16_t magic;  // always first
    int32_t hiScore;
    uint8_t crc;
};
#pragma pack(pop)
typedef struct SaveData SaveData;

//everything saved has to fit in what the platform stores
static_assert(sizeof(SaveData) <= PLATFORM_STORAGE_SIZE, "the save state does not fit in PLATFORM_STORAGE_SIZE");

SaveData saveData;

uint8_t calcCRC(void *data, size_t len) {
  uint8_t crc = 0;
  uint8_t *ptr = (uint8_t *)data;
  for (size_t i = 0; i < len; i++) {
    crc ^= ptr[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x07;
      else
        crc <<= 1;
    }
  }
  return crc;
}

void loadSaveState(void)
{
    Platform_StorageRead(0, (uint8_t*)&saveData, sizeof(SaveData));
    //needs to be -uint8t size because of crc not included in calculation
    uint8_t crc = calcCRC(&saveData, sizeof(SaveData) - sizeof(uint8_t));
    if (saveData.magic != SAVE_MAGIC || saveData.crc != crc)
    {
        Platform_Log("save state invalid, loading defaults\n");
        memset(&saveData, 0, sizeof(SaveData));
        saveData.magic = SAVE_MAGIC;
    }
    else
    {
        Platform_Log("save state valid, loaded scores\n");
    }
}

void saveSaveState(void)
{
    saveData.crc = calcCRC(&saveData, sizeof(SaveData) - sizeof(uint8_t));
    Platform_StorageWrite(0, (const uint8_t*)&saveData, sizeof(SaveData));
    Platform_Log("saved, crc: 0x%02X\n", saveData.crc);
}

uint32_t getHiScore(void)
{
    return saveData.hiScore;
}

void setHiScore(int value)
{
    saveData.hiScore = value;
}
