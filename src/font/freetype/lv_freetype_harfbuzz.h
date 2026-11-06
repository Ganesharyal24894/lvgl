/**
 * @file lv_freetype_harfbuzz.h
 *
 */

#ifndef LV_FREETYPE_HARFBUZZ_H
#define LV_FREETYPE_HARFBUZZ_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "../../lvgl_public.h"

#if LV_USE_FREETYPE && LV_USE_HARFBUZZ

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint32_t glyph_id;
    int32_t x_offset;
    int32_t y_offset;
    int32_t x_advance;
    int32_t y_advance;
    uint32_t cluster;
} lv_hb_glyph_info_t;

typedef struct {
    lv_hb_glyph_info_t * glyphs;
    uint32_t count;
} lv_hb_shaped_text_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Shape a UTF-8 text string using HarfBuzz.
 * Caller must free the result with lv_hb_shaped_text_destroy().
 * @param font pointer to an LVGL font (must be a FreeType font)
 * @param text UTF-8 encoded text
 * @param byte_len length of text in bytes
 * @param dir_hint text direction hint: LV_BASE_DIR_AUTO to let HarfBuzz detect direction,
 *                 LV_BASE_DIR_LTR or LV_BASE_DIR_RTL to force a direction (use when text
 *                 has already been BIDI-reordered to prevent double-reordering)
 * @return pointer to shaped text result, or NULL on failure
 */
lv_hb_shaped_text_t * lv_hb_shape_text(const lv_font_t * font, const char * text, uint32_t byte_len,
                                       lv_base_dir_t dir_hint);

/**
 * Free shaped text result.
 * @param shaped pointer to shaped text to free
 */
void lv_hb_shaped_text_destroy(lv_hb_shaped_text_t * shaped);

/**
 * Check whether text in this font goes through HarfBuzz shaping rather than
 * LVGL's character-by-character path. Shaping is required for scripts where
 * glyphs join, reorder or stack (Devanagari, Arabic, Thai and others) and is
 * enabled per font with lv_freetype_font_set_use_harfbuzz().
 * @param font pointer to an LVGL font
 * @return true if the font is a FreeType font with HarfBuzz enabled
 */
bool lv_freetype_is_harfbuzz_font(const lv_font_t * font);

/**********************
 *      MACROS
 **********************/

#else /* LV_USE_FREETYPE && LV_USE_HARFBUZZ */

/**
 * Always declared so callers can branch without preprocessor conditionals.
 * @param font pointer to an LVGL font
 * @return false
 */
static inline bool lv_freetype_is_harfbuzz_font(const lv_font_t * font)
{
    LV_UNUSED(font);
    return false;
}

#endif /* LV_USE_FREETYPE && LV_USE_HARFBUZZ */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LV_FREETYPE_HARFBUZZ_H */
