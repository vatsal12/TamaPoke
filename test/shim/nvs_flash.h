// Shim de <nvs_flash.h> (ESP-IDF) sobre la NVS en memoria de Preferences.h.
// La particion "tamabox" de la caja se inicializa con nvs_flash_init_partition;
// mockNvsSetInitResult() deja simular una particion ausente o sin formato.
#pragma once
#include <stdint.h>
typedef int esp_err_t;
#ifndef ESP_OK
#define ESP_OK 0
#endif
#ifndef ESP_FAIL
#define ESP_FAIL -1
#endif
#define ESP_ERR_NOT_FOUND 0x105
#define ESP_ERR_NVS_NO_FREE_PAGES 0x110d
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x1110

esp_err_t nvs_flash_init_partition(const char *label);
esp_err_t nvs_flash_erase_partition(const char *label);
esp_err_t nvs_flash_init();

// test: el siguiente(s) nvs_flash_init_partition devuelven este resultado
// (ESP_OK por defecto). "times" = cuantas llamadas lo devuelven antes de ESP_OK.
void mockNvsSetInitResult(esp_err_t err, int times = 1);
// test: la particion no existe en la tabla (init siempre falla)
void mockNvsSetPartitionMissing(bool missing);
int mockNvsEraseCount();
