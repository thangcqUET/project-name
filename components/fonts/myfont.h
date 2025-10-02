// Placeholder header for a generated LVGL font containing Vietnamese glyphs.
// Generate the real C font with lv_font_conv and place the generated .c/.h next to this file.

#pragma once

// Set to 1 if you generated a font and added myfont.c which defines 'myfont_20' or similar.
#ifndef MYFONT_PROVIDED
#define MYFONT_PROVIDED 1
#endif

#if MYFONT_PROVIDED
#include "myfont.c"
#endif

// Helper macro name - adjust if your generated font uses a different symbol
#ifndef MYFONT_SYMBOL
#define MYFONT_SYMBOL myfont
#endif

// Usage in code:
// #if MYFONT_PROVIDED
// lv_obj_set_style_text_font(label, &MYFONT_SYMBOL, 0);
// #endif
