#pragma once
#include <Arduino.h>

// Hora por Wi-Fi (NTP). La 1.75C no tiene RTC: sin esto, cada arranque en frio
// pide la hora a mano. Si hay una red guardada, al arrancar se conecta, pide la
// hora a un servidor NTP, la pasa a hora LOCAL con la zona guardada (con cambio
// de horario de verano) y apaga la radio. Tarda unos segundos; si no hay red en
// 20 s se rinde y queda la pantalla de ajuste manual de siempre.
//
// Las credenciales van en su propio espacio NVS ("wifi"), fuera de la partida:
// asi EXPORT (y las copias de seguridad) nunca llevan la contrasena.
// Se guardan con tools/wifi_setup.py (pide la contrasena sin mostrarla).

void wifiTimeBegin();                    // arranca un intento si hay red guardada
bool wifiTimePoll(uint32_t *localEpoch); // llamar en loop(): true UNA vez al sincronizar
bool wifiTimeBusy();                     // la radio esta en uso (no empezar LAN)
bool wifiSerialCommand(const String &line);  // WIFI ... (consola serie)
