#!/usr/bin/env python3
"""Copia de seguridad de la partida (y la caja) por USB, y su restauracion.

  python3 tools/save_backup.py backup                 # -> backups/tamapoke-AAAAMMDD-HHMM.txt
  python3 tools/save_backup.py restore FICHERO.txt    # la vuelve a meter (la placa reinicia)
  python3 tools/save_backup.py raw-backup             # copia en bruto de la flash (nvs + caja)
  python3 tools/save_backup.py raw-restore FICHERO.bin

backup/restore usan los comandos EXPORT / IMPORT de la consola serie: el
fichero es texto (lineas "IMPORT <hex>"), y la placa valida el bloque entero
(cabecera, version y CRC) antes de tocar nada.

raw-backup/raw-restore leen y escriben la flash con esptool, sin depender del
firmware: sirven tambien con una version vieja que no tiene EXPORT (por ejemplo
antes de actualizar a esta). Copian la particion nvs (0x9000, 20 KB: la partida)
y la tamabox (0x410000, 64 KB: la caja) a un unico .bin. raw-restore solo debe
usarse con la MISMA tabla de particiones (partitions.csv) con la que se saco.

Necesita pyserial para backup/restore (pip install pyserial) y el esptool que
trae el core de Arduino para las copias en bruto.
"""
import argparse
import datetime
import glob
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BACKUPS = os.path.join(ROOT, 'backups')   # en .gitignore: es la partida de alguien

NVS = (0x9000, 0x5000)
BOX = (0x410000, 0x10000)


def find_port(port):
    if port:
        return port
    ports = glob.glob('/dev/cu.usbmodem*') + glob.glob('/dev/ttyACM*')
    if not ports:
        sys.exit('no encuentro la placa (usa --port)')
    return ports[0]


def stamp(ext):
    os.makedirs(BACKUPS, exist_ok=True)
    return os.path.join(BACKUPS, datetime.datetime.now().strftime(f'tamapoke-%Y%m%d-%H%M.{ext}'))


def open_serial(port):
    try:
        import serial
    except ImportError:
        sys.exit('falta pyserial: pip install pyserial')
    ser = serial.Serial(port, 115200, timeout=1)
    time.sleep(0.3)
    ser.reset_input_buffer()
    return ser


def backup(args):
    ser = open_serial(find_port(args.port))
    ser.write(b'EXPORT\n')
    lines, deadline = [], time.time() + 15
    while time.time() < deadline:
        line = ser.readline().decode(errors='replace').strip()
        if not line:
            continue
        if line == 'EXPORT FAIL':
            sys.exit('la placa no pudo exportar la partida')
        if line.startswith('# TamaPoke save') or line.startswith('IMPORT'):
            lines.append(line)
            if line == 'IMPORT':          # la vacia cierra el bloque
                break
    ser.close()
    if not lines or lines[-1] != 'IMPORT':
        sys.exit('no llego la copia completa (firmware sin EXPORT? prueba raw-backup)')
    out = args.out or stamp('txt')
    with open(out, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    print(f'copia guardada en {out} ({len(lines) - 2} lineas de datos)')


def restore(args):
    with open(args.file) as f:
        lines = [ln.strip() for ln in f if ln.strip().startswith('IMPORT')]
    if not lines or lines[-1] != 'IMPORT':
        sys.exit(f'{args.file} no parece una copia de save_backup.py backup')
    ser = open_serial(find_port(args.port))
    for ln in lines:
        ser.write(ln.encode() + b'\n')
        time.sleep(0.05)
    deadline = time.time() + 10
    while time.time() < deadline:
        r = ser.readline().decode(errors='replace').strip()
        if r in ('IMPORT OK', 'IMPORT REJECTED') or r.startswith('IMPORT '):
            print(r)
            if r == 'IMPORT OK':
                print('restaurada: la placa se reinicia con la partida de la copia')
                return
            sys.exit('la placa rechazo la copia (no se ha cambiado nada)')
    sys.exit('sin respuesta de la placa')


def esptool():
    found = sorted(glob.glob(os.path.expanduser(
        '~/Library/Arduino15/packages/esp32/tools/esptool_py/*/esptool')))
    found += sorted(glob.glob(os.path.expanduser(
        '~/.arduino15/packages/esp32/tools/esptool_py/*/esptool')))
    if found:
        return [found[-1]]
    return [sys.executable, '-m', 'esptool']


def raw_backup(args):
    port = find_port(args.port)
    out = args.out or stamp('bin')
    parts = []
    for off, size in (NVS, BOX):
        tmp = f'{out}.{off:x}'
        subprocess.run(esptool() + ['--chip', 'esp32s3', '--port', port, 'read-flash',
                                    hex(off), hex(size), tmp], check=True)
        with open(tmp, 'rb') as f:
            parts.append(f.read())
        os.remove(tmp)
    with open(out, 'wb') as f:
        for p in parts:
            f.write(p)
    print(f'copia en bruto en {out} (nvs {NVS[1]} B + caja {BOX[1]} B)')


def raw_restore(args):
    port = find_port(args.port)
    data = open(args.file, 'rb').read()
    if len(data) != NVS[1] + BOX[1]:
        sys.exit(f'{args.file}: tamano inesperado ({len(data)} B)')
    tmp_n, tmp_b = args.file + '.nvs', args.file + '.box'
    with open(tmp_n, 'wb') as f:
        f.write(data[:NVS[1]])
    with open(tmp_b, 'wb') as f:
        f.write(data[NVS[1]:])
    try:
        subprocess.run(esptool() + ['--chip', 'esp32s3', '--port', port, 'write-flash',
                                    hex(NVS[0]), tmp_n, hex(BOX[0]), tmp_b], check=True)
    finally:
        os.remove(tmp_n)
        os.remove(tmp_b)
    print('restaurada la copia en bruto')


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--port')
    sub = ap.add_subparsers(dest='cmd', required=True)
    b = sub.add_parser('backup')
    b.add_argument('--out')
    r = sub.add_parser('restore')
    r.add_argument('file')
    rb = sub.add_parser('raw-backup')
    rb.add_argument('--out')
    rr = sub.add_parser('raw-restore')
    rr.add_argument('file')
    args = ap.parse_args()
    {'backup': backup, 'restore': restore, 'raw-backup': raw_backup,
     'raw-restore': raw_restore}[args.cmd](args)


if __name__ == '__main__':
    main()
