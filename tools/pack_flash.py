#!/usr/bin/env python3
"""Paquete de sprites para la particion "sprites" de la flash (placas sin microSD, 1.75C).

Lee los bundles por region que ya distribuye el instalador (web/sprites-*.pak,
formato TPAK: lo mismo que va a la SD) y genera un unico paquete TPKZ con cada
fichero comprimido con zlib. Los fotogramas son casi todo pixel transparente, asi
que los ~240 MB de las 7 regiones se quedan en ~7.5 MB. El firmware lee el indice
al arrancar y descomprime cada especie a PSRAM al cargarla (sdmon.cpp).

  python3 tools/pack_flash.py                 # -> web/firmware/175c/sprites.bin
  python3 tools/pack_flash.py --out X.bin

Formato TPKZ (little endian):
  "TPKZ" u16 count u16 version(=1)
  count entradas de 28 bytes, ORDENADAS por nombre (el firmware hace busqueda
  binaria con strncmp): char name[16] (sin "mons/", con ceros de relleno),
  u32 offset (desde el principio de la particion), u32 zlen, u32 rawLen
  y luego los streams zlib, uno tras otro.

El paquete se graba en la particion "sprites" de partitions.csv (offset 0x420000);
tools/build_web.sh lo pone en manifest-175c.json y comprueba que cabe.
"""
import argparse
import glob
import os
import struct
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAKS = os.path.join(ROOT, 'web', 'sprites-*.pak')
OUT = os.path.join(ROOT, 'web', 'firmware', '175c', 'sprites.bin')

NAME_MAX = 15            # PackEntry::name es char[16]
ENTRY = struct.Struct('<16sIII')
HEADER = 8
PARTITION_SIZE = 0xBD0000  # particion "sprites" de partitions.csv


def read_pak(path):
    """Ficheros de un bundle TPAK: lista de (nombre, bytes)."""
    b = open(path, 'rb').read()
    if b[:4] != b'TPAK':
        raise SystemExit(f'{path}: no es un bundle TPAK')
    n = struct.unpack_from('<H', b, 4)[0]
    off, items = 6, []
    for _ in range(n):
        nl = b[off]
        off += 1
        name = b[off:off + nl].decode()
        off += nl
        size = struct.unpack_from('<I', b, off)[0]
        off += 4
        items.append((name, size))
    out = []
    for name, size in items:
        out.append((name, b[off:off + size]))
        off += size
    if off != len(b):
        raise SystemExit(f'{path}: tamano inesperado ({off} != {len(b)})')
    return out


def collect(paks):
    """Une los bundles. Un mismo nombre en dos bundles tiene que ser identico
    (pasa con thumbs.bin, que cada region lleva entero): se guarda una vez."""
    files = {}
    for p in sorted(paks):
        for name, data in read_pak(p):
            if name.startswith('mons/'):
                name = name[5:]
            if len(name.encode()) > NAME_MAX:
                raise SystemExit(f'{name}: nombre de mas de {NAME_MAX} caracteres')
            if name in files and files[name] != data:
                raise SystemExit(f'{name}: distinto en dos bundles')
            files[name] = data
    if not files:
        raise SystemExit(f'no hay bundles en {PAKS} (ejecuta tools/pack_bundle.py)')
    return files


def build(files):
    names = sorted(files, key=lambda n: n.encode())  # orden de bytes = strncmp
    blobs = [zlib.compress(files[n], 9) for n in names]
    data_off = HEADER + ENTRY.size * len(names)
    index, off = [], data_off
    for n, z in zip(names, blobs):
        index.append(ENTRY.pack(n.encode(), off, len(z), len(files[n])))
        off += len(z)
    out = b'TPKZ' + struct.pack('<HH', len(names), 1) + b''.join(index) + b''.join(blobs)
    if len(out) > PARTITION_SIZE:
        raise SystemExit(f'el paquete ({len(out)} bytes) no cabe en la particion ({PARTITION_SIZE})')
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--out', default=OUT)
    args = ap.parse_args()
    files = collect(glob.glob(PAKS))
    out = build(files)
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, 'wb') as f:
        f.write(out)
    raw = sum(len(d) for d in files.values())
    print(f'{args.out}: {len(files)} ficheros, {raw / 1e6:.1f} MB -> {len(out) / 1e6:.2f} MB '
          f'({len(out) * 100 // PARTITION_SIZE}% de la particion)')


if __name__ == '__main__':
    main()
