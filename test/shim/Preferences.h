// Shim de la API Preferences (NVS de ESP32) en memoria.
// Comparte un unico almacen global entre instancias, igual que la NVS real,
// para poder testear guardado/carga y persistencia entre "arranques".
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>

class Preferences {
public:
  // partitionLabel: como en Arduino-ESP32, el namespace vive dentro de una
  // particion NVS ("nvs" por defecto). Dos particiones son almacenes separados:
  // el mismo namespace en "nvs" y en "tamabox" no comparte claves.
  bool begin(const char *name, bool readOnly = false, const char *partitionLabel = nullptr);
  void end();
  bool clear();
  bool isKey(const char *key);
  bool remove(const char *key);

  size_t putBool(const char *k, bool v);
  size_t putUChar(const char *k, uint8_t v);
  size_t putChar(const char *k, int8_t v);
  size_t putShort(const char *k, int16_t v);
  size_t putUShort(const char *k, uint16_t v);
  size_t putInt(const char *k, int32_t v);
  size_t putUInt(const char *k, uint32_t v);
  size_t putBytes(const char *k, const void *v, size_t len);
  size_t putString(const char *k, const char *v);

  bool getBool(const char *k, bool def = false);
  uint8_t getUChar(const char *k, uint8_t def = 0);
  int8_t getChar(const char *k, int8_t def = 0);
  int16_t getShort(const char *k, int16_t def = 0);
  uint16_t getUShort(const char *k, uint16_t def = 0);
  int32_t getInt(const char *k, int32_t def = 0);
  uint32_t getUInt(const char *k, uint32_t def = 0);
  size_t getBytesLength(const char *k);
  // COMO EL HARDWARE: si el blob guardado es MAS LARGO que maxLen no copia
  // nada y devuelve 0 (Arduino-ESP32: "not enough space in buffer").
  size_t getBytes(const char *k, void *buf, size_t maxLen);
  size_t getString(const char *k, char *buf, size_t maxLen);

private:
  std::string ns_;
  bool open_ = false;
  bool ro_ = false;
};

// helpers de test: borra toda la "NVS" simulada (todas las particiones)
void mockNvsReset();
size_t mockNvsKeyCount(const char *ns, const char *partition = "nvs");
bool mockNvsHasKey(const char *ns, const char *key, const char *partition = "nvs");
// borra una particion entera (lo usa el shim de nvs_flash_erase_partition)
void mockNvsErasePartition(const char *partition);
