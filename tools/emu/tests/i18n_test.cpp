// Checks the positional STRINGS table: a language row with too few entries is
// zero-padded by the compiler with no diagnostic, which silently shifts every
// string after the gap. Also enforces the one-byte-per-character rule: the
// bitmap font is CP437 (accents are single high bytes), and JA/KO are UTF-8 for
// their U8g2 font.
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

// STRINGS is file-static, so go through the same accessor the firmware uses.
// Sized by LANG_COUNT so a new language is a compile error here, not a read
// past the end of the array the first time one of its strings is reported.
static const char *LANG[LANG_COUNT] = { "ES", "EN", "FR", "DE", "IT", "PT", "JA", "KO" };

// JA/KO are UTF-8 on purpose (they draw with a U8g2 font, applyLangFont), so
// for them the rule is "well-formed UTF-8", not "ASCII".
static bool validUtf8(const unsigned char *p) {
  while (*p) {
    int n = *p < 0x80 ? 0 : (*p & 0xE0) == 0xC0 ? 1 : (*p & 0xF0) == 0xE0 ? 2 : (*p & 0xF8) == 0xF0 ? 3 : -1;
    if (n < 0) return false;
    p++;
    for (int i = 0; i < n; i++, p++) if ((*p & 0xC0) != 0x80) return false;
  }
  return true;
}

int main() {
  int bad = 0;
  for (int l = 0; l < LANG_COUNT; l++)
    for (int s = 0; s < STR_COUNT; s++) {
      setLang((Lang)l);
      const char *v = T((StrId)s);
      if (!v) { printf("NULL  %s index %d\n", LANG[l], s); bad++; continue; }
      if (LANG_IS_CJK(l)) {
        if (!validUtf8((const unsigned char *)v)) { printf("BAD UTF-8  %s index %d\n", LANG[l], s); bad++; }
        continue;
      }
      // The classic font is a 256-glyph CP437 table, so accents ARE allowed --
      // as ONE byte each. UTF-8 is what is banned: the UI centres with
      // strlen()*6, and a two-byte character draws as two glyphs and pushes the
      // label off centre. A UTF-8 lead byte followed by a continuation byte is
      // the tell (same rule as test/test_i18n.cpp on the firmware side).
      for (const unsigned char *p = (const unsigned char *)v; *p; p++) {
        bool lead = (*p >= 0xC2 && *p <= 0xDF) || (*p >= 0xE0 && *p <= 0xEF) || (*p >= 0xF0 && *p <= 0xF4);
        if (lead && p[1] >= 0x80 && p[1] <= 0xBF) {
          printf("UTF-8  %s index %d: \"%s\"\n", LANG[l], s, v); bad++; break;
        }
      }
    }
  printf("%s: %d languages x %d strings\n", bad ? "FAIL" : "PASS", LANG_COUNT, STR_COUNT);
  return bad ? 1 : 0;
}
