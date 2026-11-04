#!/bin/bash
# Regenera el firmware del instalador web, en CUATRO piezas por placa:
#   web/firmware/175/   -> ESP32-S3-Touch-AMOLED-1.75  (sprites en la microSD)
#   web/firmware/175c/  -> ESP32-S3-Touch-AMOLED-1.75C (sprites dentro del firmware)
# cada una con bootloader.bin (0x0), partitions.bin (0x8000), boot_app0.bin
# (0xe000) y app.bin (0x10000).
#
# NO se publica una imagen combinada: esptool merge-bin rellena los huecos con
# 0xFF y escribiria en blanco la particion NVS (0x9000-0xE000), que es donde
# vive la partida. Con piezas sueltas la NVS cae en el hueco y no se toca, igual
# que al subir desde el IDE de Arduino. test/test_tools.py lo comprueba.
#
# Uso: bash tools/build_web.sh
set -e
cd "$(dirname "$0")/.."
FQBN="esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB"

# la 1.75C compila los sprites de tools/sdcard/mons dentro del firmware
python3 tools/pack_flash.py

build() {  # build <carpeta> [flags]
  local dir="$1" flags="$2" B="build/$1" OUT="web/firmware/$1"
  echo "Compilando $dir..."
  arduino-cli compile --fqbn "$FQBN" --build-property "compiler.cpp.extra_flags=$flags" \
    --build-path "$B" .
  mkdir -p "$OUT"
  cp "$B/TamaPoke.ino.bootloader.bin" "$OUT/bootloader.bin"
  cp "$B/TamaPoke.ino.partitions.bin" "$OUT/partitions.bin"
  cp "$B/boot_app0.bin" "$OUT/boot_app0.bin"
  cp "$B/TamaPoke.ino.bin" "$OUT/app.bin"
  echo "OK -> $OUT ($(du -ch "$OUT"/*.bin | tail -1 | cut -f1))"
}

build 175 ""
build 175c "-DTAMAPOKE_BOARD_175C"

echo "Comprobando que el instalador no pisa la partida..."
python3 test/test_tools.py TestInstalador

echo "Empaquetando sprites (solo para la SD de la 1.75)..."
python3 tools/pack_bundle.py
