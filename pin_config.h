#pragma once

// ---------------------------------------------------------------------------
// Board selection
//
// Default: Waveshare ESP32-S3-Touch-AMOLED-1.75 (the original board, with
// PCF85063 RTC and microSD slot).
//
// For the ESP32-S3-Touch-AMOLED-1.75C (aluminium case, 32 MB flash, no RTC,
// no microSD) uncomment the next line, or pass -DTAMAPOKE_BOARD_175C to the
// compiler (arduino-cli: --build-property compiler.cpp.extra_flags=-DTAMAPOKE_BOARD_175C).
// ---------------------------------------------------------------------------
// #define TAMAPOKE_BOARD_175C

#define XPOWERS_CHIP_AXP2101

// Pantalla AMOLED 466x466, driver CO5300 por QSPI (igual en las dos placas)
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_WIDTH 466
#define LCD_HEIGHT 466

// Táctil capacitivo CST9217 por I2C (bus compartido con el AXP2101)
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT 11

// Audio ES8311 (BCLK/WS/DIN/DOUT y amplificador iguales en las dos placas)
#define I2S_BCK_IO 9
#define I2S_DI_IO 10
#define I2S_WS_IO 45
#define I2S_DO_IO 8
#define PA 46

#ifdef TAMAPOKE_BOARD_175C
// ---- ESP32-S3-Touch-AMOLED-1.75C ----
// Fuente: github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C
// (examples/arduino/libraries/board_config/pin_config.h) y su esquematico.
// Los resets del panel y del tactil estan en GPIO1/GPIO2, que en la 1.75 eran
// la ranura TF: con el firmware de la 1.75 el panel nunca sale de reset
// (pantalla negra).
#define BOARD_NAME "1.75C"
#define LCD_RESET 1
#define TP_RESET 2
#define I2S_MCK_IO 16
#define TAMAPOKE_HAS_RTC 0         // sin PCF85063: reloj por software (rtcbat.cpp)
#define TAMAPOKE_HAS_SD 0          // sin ranura TF
#define TAMAPOKE_FLASH_SPRITES 1   // sprites comprimidos dentro del firmware (flashsprites.cpp)
#else
// ---- ESP32-S3-Touch-AMOLED-1.75 (original) ----
// Fuente: github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75 (libraries/Mylibrary/pin_config.h)
#define BOARD_NAME "1.75"
#define LCD_RESET 39
#define TP_RESET 40
// NOTA: el MCLK real es GPIO42 (verificado en placa con el proyecto
// PlaneRadar2.0); el 16 que figuraba era erroneo. Aun asi el codec se
// configura con reloj derivado del BCLK, asi que el MCLK apenas importa.
#define I2S_MCK_IO 42
#define TAMAPOKE_HAS_RTC 1
#define TAMAPOKE_HAS_SD 1
#define TAMAPOKE_FLASH_SPRITES 0

// Ranura TF
#define SDMMC_CLK 2
#define SDMMC_CMD 1
#define SDMMC_DATA 3
#define SDMMC_CS 41
#endif
