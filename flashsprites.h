#pragma once
#include <stdint.h>

// Sprites dentro del firmware (placas sin microSD, ver pin_config.h).
// Los datos los genera tools/pack_flash.py en flashsprites.cpp: cada fichero de
// tools/sdcard/mons comprimido con zlib, en flash (rodata, no ocupa RAM).
struct FlashSprite {
  char name[16];    // "p001.bin", "ps025.bin", "thumbs.bin" (sin /mons/)
  uint32_t offset;  // en FLASH_SPRITES_DATA
  uint32_t zlen;    // bytes comprimidos (stream zlib con cabecera)
  uint32_t rawLen;  // bytes una vez descomprimido
};

extern const FlashSprite FLASH_SPRITES[];
extern const uint16_t FLASH_SPRITES_COUNT;
extern const uint8_t FLASH_SPRITES_DATA[];
