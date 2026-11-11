#!/usr/bin/env python3
"""Guarda tu red Wi-Fi en la placa para que ponga la hora sola al arrancar.

  python3 tools/wifi_setup.py            # pregunta la red y la contrasena (oculta)
  python3 tools/wifi_setup.py --forget   # borra la red guardada

La 1.75C no tiene reloj (RTC): sin esto, tras apagarla del todo pide la hora a
mano. Con una red guardada, al arrancar se conecta unos segundos, pide la hora
(NTP), la pone en hora local y apaga la radio. No se queda conectada.

La contrasena se escribe sin que se vea y va directa a la placa por USB; se
guarda en su propio espacio de la flash ("wifi"), fuera de la partida, asi que
las copias de save_backup.py no la llevan. Necesita pyserial.
"""
import argparse
import getpass
import glob
import sys
import time

ZONES = {
    '1': ('Pacific (Vancouver/LA)', 'PST8PDT,M3.2.0,M11.1.0'),
    '2': ('Mountain (Calgary/Denver)', 'MST7MDT,M3.2.0,M11.1.0'),
    '3': ('Central (Chicago/Winnipeg)', 'CST6CDT,M3.2.0,M11.1.0'),
    '4': ('Eastern (Toronto/New York)', 'EST5EDT,M3.2.0,M11.1.0'),
    '5': ('UK (London)', 'GMT0BST,M3.5.0/1,M10.5.0'),
    '6': ('Central Europe (Madrid/Berlin)', 'CET-1CEST,M3.5.0,M10.5.0/3'),
    '7': ('India (IST)', 'IST-5:30'),
}
SEP = '\x1f'   # separador que el firmware espera (deja usar espacios)


def find_port(port):
    if port:
        return port
    ports = glob.glob('/dev/cu.usbmodem*') + glob.glob('/dev/ttyACM*')
    if not ports:
        sys.exit('no encuentro la placa (usa --port)')
    return ports[0]


def talk(ser, cmd, wait=6.0, until=('DONE',)):
    ser.write(cmd.encode() + b'\n')
    out, end = [], time.time() + wait
    while time.time() < end:
        line = ser.readline().decode(errors='replace').strip()
        if not line:
            continue
        if line.startswith('wifi:'):
            print(line)
        out.append(line)
        if line in until:
            break
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--port')
    ap.add_argument('--forget', action='store_true')
    args = ap.parse_args()
    try:
        import serial
    except ImportError:
        sys.exit('falta pyserial: pip install pyserial')
    ser = serial.Serial(find_port(args.port), 115200, timeout=0.5)
    time.sleep(0.3)
    ser.reset_input_buffer()
    if args.forget:
        talk(ser, 'WIFI FORGET')
        return
    ssid = input('Wi-Fi network name (SSID): ').strip()
    pw = getpass.getpass('Wi-Fi password (hidden): ')
    if not ssid or len(ssid) > 32 or len(pw) > 64 or SEP in ssid + pw:
        sys.exit('network name or password not valid')
    print('Timezone:')
    for k, (name, _) in ZONES.items():
        print(f'  {k}. {name}')
    tz = ZONES.get(input('Choose [1]: ').strip() or '1', ZONES['1'])[1]
    talk(ser, f'WIFISET {ssid}{SEP}{pw}{SEP}{tz}')
    del pw
    print('Getting the time (up to 20 s)...')
    end = time.time() + 25
    while time.time() < end:
        line = ser.readline().decode(errors='replace').strip()
        if line.startswith('wifi:'):
            print(line)
            if 'hora local' in line:
                print('Done: the board will set its clock by Wi-Fi every time it starts.')
                return
            if 'sin hora' in line:
                sys.exit('Could not get the time: check the network name and password, '
                         'and that it is a 2.4 GHz network.')
    sys.exit('No answer from the board.')


if __name__ == '__main__':
    main()
