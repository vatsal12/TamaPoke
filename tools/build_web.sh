#!/bin/bash
# Regenera el firmware del instalador web, en piezas sueltas por placa:
#   web/firmware/175/   -> ESP32-S3-Touch-AMOLED-1.75  (sprites en la microSD)
#   web/firmware/175c/  -> ESP32-S3-Touch-AMOLED-1.75C (sprites en la particion "sprites")
# cada una con bootloader.bin (0x0), partitions.bin (0x8000), boot_app0.bin
# (0xe000) y app.bin (0x10000); la 1.75C ademas sprites.bin (0x420000).
#
# NO se publica una imagen combinada: esptool merge-bin rellena los huecos con
# 0xFF y escribiria en blanco la particion NVS (0x9000-0xE000), que es donde
# vive la partida, y la "tamabox" (0x410000), que es la caja. Con piezas sueltas
# esas dos caen en huecos y no se tocan, igual que al subir desde el IDE de
# Arduino. test/test_tools.py (TestInstalador) lo comprueba y para el script.
#
# La tabla de particiones es partitions.csv (en la carpeta del sketch), por eso
# PartitionScheme=custom: con app3M_fat9M_16MB arduino-cli comprobaria el
# tamano contra 3 MB aunque la particion de la app ya sea de 4.
#
# Uso: bash tools/build_web.sh
set -e
cd "$(dirname "$0")/.."
ROOT="$PWD"
FQBN="esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=custom"

# arduino-cli exige que la carpeta se llame como el sketch (TamaPoke). En un
# worktree o una copia con otro nombre se compila desde una copia temporal.
SRC="$ROOT"
if [ "$(basename "$ROOT")" != "TamaPoke" ]; then
  TMP="$(mktemp -d)"
  trap 'rm -rf "$TMP"' EXIT
  SRC="$TMP/TamaPoke"
  mkdir -p "$SRC"
  rsync -a --exclude .git --exclude web --exclude docs --exclude tools --exclude test \
    --exclude build --exclude .claude --exclude .impeccable "$ROOT/" "$SRC/"
fi

build() {  # build <carpeta> [flags]
  local dir="$1" flags="$2" B="$ROOT/build/$1" OUT="$ROOT/web/firmware/$1"
  echo "Compilando $dir..."
  arduino-cli compile --fqbn "$FQBN" --build-property "compiler.cpp.extra_flags=$flags" \
    --build-path "$B" "$SRC"
  mkdir -p "$OUT"
  cp "$B/TamaPoke.ino.bootloader.bin" "$OUT/bootloader.bin"
  cp "$B/TamaPoke.ino.partitions.bin" "$OUT/partitions.bin"
  cp "$B/boot_app0.bin" "$OUT/boot_app0.bin"
  cp "$B/TamaPoke.ino.bin" "$OUT/app.bin"
  echo "OK -> $OUT"
}

build 175 ""
build 175c "-DTAMAPOKE_BOARD_175C"

echo "Paquete de sprites de la 1.75C (todas las regiones, de web/sprites-*.pak)..."
python3 tools/pack_flash.py

# los manifiestos: la version sale de FW_VERSION para que no se desfase a mano
FW="$(grep -o '#define FW_VERSION "[^"]*"' TamaPoke.ino | head -1 | cut -d'"' -f2)"
python3 - "$FW" <<'PYEOF'
import json, sys
fw = sys.argv[1]
def manifest(name, board_dir, extra):
    parts = [
        {'path': f'firmware/{board_dir}/bootloader.bin', 'offset': 0x0},
        {'path': f'firmware/{board_dir}/partitions.bin', 'offset': 0x8000},
        {'path': f'firmware/{board_dir}/boot_app0.bin',  'offset': 0xe000},
        {'path': f'firmware/{board_dir}/app.bin',        'offset': 0x10000},
    ] + extra
    m = {
        'name': name,
        'version': fw,
        # SIEMPRE true, y el nombre dice lo contrario de lo que hace: false =
        # "no preguntes, borra el chip entero" (NVS incluida). true = pantalla
        # con la casilla "Erase device" DESMARCADA; sin marcarla se instala sin
        # borrar y, con estas piezas, la partida y la caja quedan intactas.
        'new_install_prompt_erase': True,
        'builds': [{'chipFamily': 'ESP32-S3', 'parts': parts}],
    }
    return m
out = {
    'web/manifest.json': manifest('TamaPoke', '175', []),
    'web/manifest-175c.json': manifest('TamaPoke (1.75C)', '175c',
        [{'path': 'firmware/175c/sprites.bin', 'offset': 0x420000}]),
}
for path, m in out.items():
    with open(path, 'w') as f:
        json.dump(m, f, indent=2)
        f.write('\n')
    print(f'{path}: v{fw}')
PYEOF

echo "Comprobando que el instalador no pisa la partida ni la caja..."
python3 test/test_tools.py TestInstalador

# los .pak por region (para la microSD de la 1.75) solo se rehacen si estan
# los sprites sueltos en tools/sdcard/mons (no van en el repo: ver .gitignore)
if ls tools/sdcard/mons/p*.bin >/dev/null 2>&1; then
  echo "Empaquetando sprites por region (microSD de la 1.75)..."
  python3 tools/pack_bundle.py
else
  echo "(sin tools/sdcard/mons: se dejan los web/sprites-*.pak como estan)"
fi
