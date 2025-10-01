#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize LVGL and the e-paper display bridge. Call once at startup.
void lvgl_epaper_init(void);


// Draw an LVGL image descriptor directly to the e-paper display.
// Accepts an lv_img_dsc_t pointer (LVGL v9 image descriptor). The function will
// convert the image to the display's 1-bit framebuffer if necessary and push it.
// Returns true on success.
// Provide an LVGL image descriptor as a raw memory buffer (pointer to an
// lv_img_dsc_t instance). The function will cast the buffer to an
// lv_img_dsc_t and perform conversion/push. buf_size is the size of the
// buffer in bytes (caller responsibility to ensure it's valid).
bool lvgl_epaper_draw_lv_image(const uint8_t* buf, size_t buf_size);

#ifdef __cplusplus
}
#endif
