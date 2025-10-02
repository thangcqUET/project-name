# Generate LVGL font with Vietnamese glyphs

This folder contains a helper PowerShell script `generate_font.ps1` that calls `lv_font_conv` (via `npx`) to convert a TrueType font (TTF) to an LVGL C font that includes Vietnamese glyph ranges.

Steps:

1. Install Node.js and ensure `npx` is available.
2. Place a TTF that supports Vietnamese (for example NotoSans-Regular.ttf) somewhere on disk.
3. From PowerShell run:

```powershell
cd components/fonts
./generate_font.ps1 -ttfPath "C:\path\to\NotoSans-Regular.ttf" -size 20 -outName myfont
```

This creates `myfont.c` and `myfont.h` in the same folder. After generation:

- Edit `myfont.h` (the placeholder) or set `#define MYFONT_PROVIDED 1` so `main.cpp` will pick up the symbol.
- Confirm the font symbol name in the generated header (e.g. `myfont_20`) and set `MYFONT_SYMBOL` in `components/fonts/myfont.h` if needed.

Notes:
- The script requests ranges covering Latin, Latin-1, Latin Extended-A and common Vietnamese precomposed code points. Adjust ranges if you need more characters.
- `--no-compress` is used to keep the output simple; for smaller flash size you can enable compression if your project supports it.
