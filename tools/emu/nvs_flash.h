// The Preferences stub is one global key/value map that ignores namespaces and
// partitions, so there is nothing to initialise or erase per partition.
#pragma once
typedef int esp_err_t;
#ifndef ESP_OK
#define ESP_OK 0
#endif
#define ESP_ERR_NVS_NO_FREE_PAGES 0x110d
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x1110
inline esp_err_t nvs_flash_init_partition(const char *) { return ESP_OK; }
inline esp_err_t nvs_flash_erase_partition(const char *) { return ESP_OK; }
inline esp_err_t nvs_flash_init() { return ESP_OK; }
