// The firmware pulls U8g2 in only for its CJK fonts (applyLangFont() in the
// sketch). The emulator has no U8g2 renderer: the font tables are opaque bytes
// here and Arduino_Canvas::setFont() ignores them, so Japanese and Korean draw
// with the classic 5x7 glyphs (mojibake, but layout and logic still run).
#pragma once
#include <cstdint>
static const uint8_t u8g2_font_unifont_t_japanese3[] = { 0 };
static const uint8_t u8g2_font_unifont_t_korean2[] = { 0 };
