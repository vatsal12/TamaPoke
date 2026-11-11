#include "Arduino.h"
#include "Preferences.h"
// linked against the same core as every other suite, so it needs the same
// hardware stubs even though it only exercises the string table
uint32_t g_seed = 1;
FakeSerial Serial; FakeESP ESP; FakeWire Wire;
volatile int g_touchX = 0, g_touchY = 0;
volatile bool g_touchDown = false;
bool wasPressed = false;
uint32_t millis() { return 0; }
void FakeESP::restart() { exit(0); }
int FakeSerial::available() { return 0; }
String FakeSerial::readStringUntil(char) { return String(""); }
void sfxPlay(uint8_t) {}
#include "i18n.h"
#include <cstdio>
#include <cstring>
int main(){
  const StrId ids[] = { S_VIN, S_STAT_ATK, S_STAT_DEF, S_STAT_SPE, S_STAT_VIT, S_STAT_WGT };
  const char *ln[LANG_COUNT] = {"ES","EN","FR","DE","IT","PT","JA","KO"};
  int worst = 0, missing = 0;
  for (int l=0;l<LANG_COUNT;l++){ setLang((Lang)l);
    for (auto id : ids){
      // a short language row leaves NULLs at its tail; report it, do not strlen it
      if (!T(id)) { printf("MISSING %s string %d\n", ln[l], (int)id); missing++; continue; }
      // classic font: 12 px a byte at size 2. CJK (unifont, size 2 halved to 1
      // by setSize): 8 px an ASCII character, 16 px a wide glyph -- strlen()
      // would count each kana three times.
      int w = 70;
      if (LANG_IS_CJK(l)) {
        for (const unsigned char *p = (const unsigned char *)T(id); *p; p++)
          if ((*p & 0xC0) != 0x80) w += *p < 0x80 ? 8 : 16;
      } else w += (int)strlen(T(id))*12;
      if (w > 132) printf("COLLIDES %s \"%s\" ends at x=%d (bar starts 132)\n", ln[l], T(id), w);
      if (w > worst) worst = w; } }
  printf("widest label ends at x=%d\n", worst);
  return missing ? 1 : 0;
}
