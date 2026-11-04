#!/usr/bin/env python3
"""Mete los sprites dentro del firmware, para placas sin microSD (1.75C).

Lee tools/sdcard/mons/ (pNNN.bin, psNNN.bin, thumbs.bin: lo mismo que va a la SD)
y genera flashsprites.cpp con cada fichero comprimido con zlib. Los fotogramas son
casi todo pixel transparente, asi que los ~42 MB de la SD se quedan en ~1.2 MB de
flash. El firmware los descomprime a PSRAM al cargar cada especie (sdmon.cpp).

  python3 tools/pack_flash.py

El .cpp solo compila con TAMAPOKE_FLASH_SPRITES (ver pin_config.h); en la 1.75
original no ocupa nada.
"""
import os
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'tools', 'sdcard', 'mons')
OUT = os.path.join(ROOT, 'flashsprites.cpp')

NAME_MAX = 15  # FlashSprite::name es char[16]


def collect(src=SRC):
    """Ficheros que van a flash, en orden estable: sprites PMD y miniaturas."""
    names = sorted(n for n in os.listdir(src)
                   if (n.startswith('p') and n.endswith('.bin')) or n == 'thumbs.bin')
    if not names:
        raise SystemExit(f'no hay sprites en {src} (ejecuta tools/pack_pmd.py)')
    return names


def build(src=SRC):
    entries, blob = [], bytearray()
    for name in collect(src):
        if len(name) > NAME_MAX:
            raise SystemExit(f'nombre demasiado largo para FlashSprite: {name}')
        with open(os.path.join(src, name), 'rb') as fh:
            raw = fh.read()
        z = zlib.compress(raw, 9)
        entries.append((name, len(blob), len(z), len(raw)))
        blob += z
    return entries, bytes(blob)


def emit(entries, blob):
    out = []
    out.append('// GENERADO por tools/pack_flash.py - no editar; regenera con:')
    out.append('//   python3 tools/pack_flash.py')
    out.append('//')
    out.append(f'// {len(entries)} ficheros de tools/sdcard/mons, '
               f'{sum(e[3] for e in entries)} bytes -> {len(blob)} bytes con zlib.')
    out.append('#include "pin_config.h"')
    out.append('#if TAMAPOKE_FLASH_SPRITES')
    out.append('#include "flashsprites.h"')
    out.append('')
    out.append('const FlashSprite FLASH_SPRITES[] = {')
    for name, off, zlen, rawlen in entries:
        out.append(f'  {{"{name}", {off}u, {zlen}u, {rawlen}u}},')
    out.append('};')
    out.append(f'const uint16_t FLASH_SPRITES_COUNT = {len(entries)};')
    out.append('')
    out.append(f'const uint8_t FLASH_SPRITES_DATA[{len(blob)}] = {{')
    for i in range(0, len(blob), 40):
        out.append(','.join(str(b) for b in blob[i:i + 40]) + ',')
    out.append('};')
    out.append('#endif  // TAMAPOKE_FLASH_SPRITES')
    return '\n'.join(out) + '\n'


def main():
    entries, blob = build()
    with open(OUT, 'w') as fh:
        fh.write(emit(entries, blob))
    raw = sum(e[3] for e in entries)
    print(f'{len(entries)} ficheros, {raw / 1e6:.1f} MB -> {len(blob) / 1e6:.2f} MB '
          f'-> {os.path.relpath(OUT, ROOT)}')


if __name__ == '__main__':
    sys.exit(main())
