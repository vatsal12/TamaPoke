#include "wifitime.h"
#include <WiFi.h>
#include <Preferences.h>
#include <time.h>

// Zona por defecto si no se guardo otra: Pacifico (Vancouver/Los Angeles), con
// el horario de verano de Norteamerica (2o domingo de marzo - 1er domingo de
// noviembre). Formato POSIX TZ; tools/wifi_setup.py ofrece otras.
static const char *DEFAULT_TZ = "PST8PDT,M3.2.0,M11.1.0";
#define WIFI_TIMEOUT_MS 20000UL

enum : uint8_t { WT_IDLE = 0, WT_TRYING, WT_DONE, WT_FAILED };
static uint8_t gState = WT_IDLE;
static uint32_t gStarted = 0;
static char gSsid[33] = "";
static char gPass[65] = "";
static char gTz[48] = "";

static void loadCreds() {
  Preferences p;
  gSsid[0] = gPass[0] = 0;
  strncpy(gTz, DEFAULT_TZ, sizeof(gTz) - 1);
  if (!p.begin("wifi", true)) return;   // espacio propio: nunca entra en EXPORT
  p.getString("ssid", gSsid, sizeof(gSsid));
  p.getString("pass", gPass, sizeof(gPass));
  if (p.isKey("tz")) p.getString("tz", gTz, sizeof(gTz));
  p.end();
}

static void radioOff() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// dias desde 1970-01-01 de una fecha civil (Howard Hinnant, sin tablas)
static int32_t daysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int32_t)doe - 719468;
}

static void startAttempt() {
  loadCreds();
  if (!gSsid[0]) { gState = WT_IDLE; return; }
  WiFi.mode(WIFI_STA);
  WiFi.begin(gSsid, gPass);
  // configTzTime fija TZ y arranca SNTP; el reloj del sistema queda en UTC
  configTzTime(gTz, "pool.ntp.org", "time.google.com", "time.nist.gov");
  gStarted = millis();
  gState = WT_TRYING;
  Serial.printf("wifi: buscando la hora por '%s'...\n", gSsid);
}

void wifiTimeBegin() { startAttempt(); }

bool wifiTimeBusy() { return gState == WT_TRYING; }

bool wifiTimePoll(uint32_t *localEpoch) {
  if (gState != WT_TRYING) return false;
  time_t now = time(nullptr);
  if (now > 1700000000) {              // SNTP ya contesto (antes vale ~segundos desde el arranque)
    struct tm lt;
    localtime_r(&now, &lt);
    // el juego guarda la hora LOCAL como si fuera un epoch (openClock/applyClock
    // la usan tal cual), asi que se rearma desde la fecha y hora locales
    uint32_t e = (uint32_t)daysFromCivil(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday) * 86400UL +
                 (uint32_t)lt.tm_hour * 3600UL + (uint32_t)lt.tm_min * 60UL + (uint32_t)lt.tm_sec;
    radioOff();
    gState = WT_DONE;
    Serial.printf("wifi: hora local %02d:%02d (%04d-%02d-%02d)\n", lt.tm_hour, lt.tm_min,
                  lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday);
    *localEpoch = e;
    return true;
  }
  if (millis() - gStarted > WIFI_TIMEOUT_MS) {
    Serial.printf("wifi: sin hora en %lus (%s): queda el ajuste manual\n",
                  WIFI_TIMEOUT_MS / 1000,
                  WiFi.status() == WL_CONNECTED ? "conectado, NTP no contesta" : "no conecta a la red");
    radioOff();
    gState = WT_FAILED;
  }
  return false;
}

// WIFI                         estado (nunca muestra la contrasena)
// WIFISET <ssid>\x1f<pass>[\x1f<tz>]   guarda la red (lo manda tools/wifi_setup.py;
//                              el separador 0x1F deja usar espacios en ambos)
// WIFI SYNC                    pide la hora ahora
// WIFI FORGET                  borra la red guardada
bool wifiSerialCommand(const String &line) {
  if (line.startsWith("WIFISET ")) {
    String rest = line.substring(8);
    int a = rest.indexOf('\x1f');
    if (a <= 0) { Serial.println("ERR"); Serial.println("DONE"); return true; }
    int b = rest.indexOf('\x1f', a + 1);
    String ssid = rest.substring(0, a);
    String pass = b < 0 ? rest.substring(a + 1) : rest.substring(a + 1, b);
    String tz = b < 0 ? String("") : rest.substring(b + 1);
    if (ssid.length() > 32 || pass.length() > 64 || tz.length() > 47) {
      Serial.println("ERR");
      Serial.println("DONE");
      return true;
    }
    Preferences p;
    p.begin("wifi", false);
    p.putString("ssid", ssid);
    p.putString("pass", pass);
    if (tz.length()) p.putString("tz", tz);
    p.end();
    Serial.printf("wifi: red '%s' guardada\n", ssid.c_str());
    startAttempt();
    Serial.println("DONE");
    return true;
  }
  if (line == "WIFI" || line == "WIFI SYNC" || line == "WIFI FORGET") {
    if (line == "WIFI FORGET") {
      Preferences p;
      p.begin("wifi", false);
      p.clear();
      p.end();
      if (gState == WT_TRYING) radioOff();
      gState = WT_IDLE;
      Serial.println("wifi: red borrada");
    } else if (line == "WIFI SYNC") {
      startAttempt();
      if (gState == WT_IDLE) Serial.println("wifi: no hay red guardada (tools/wifi_setup.py)");
    } else {
      loadCreds();
      static const char *ST[] = { "en reposo", "buscando hora", "hora puesta", "sin hora" };
      Serial.printf("wifi: red=%s zona=%s estado=%s\n", gSsid[0] ? gSsid : "(ninguna)", gTz,
                    ST[gState]);
    }
    Serial.println("DONE");
    return true;
  }
  return false;
}
