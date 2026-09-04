#if LV_BUILD_TEST
#include "lvgl.h"
#include "src/font/freetype/lv_freetype_private.h"
#include "src/font/freetype/lv_freetype_harfbuzz.h"

#include "unity/unity.h"

#if LV_USE_FREETYPE && LV_USE_HARFBUZZ

#ifndef NON_AMD64_BUILD
    #define EXT_NAME ".lp64.png"
#else
    #define EXT_NAME ".lp32.png"
#endif

#define DEVANAGARI_FONT_PATH "../examples/libs/harfbuzz/NotoSansDevanagari-Regular.subset.ttf"

static lv_font_t * font_devanagari;

void setUp(void)
{
    font_devanagari = lv_freetype_font_create(DEVANAGARI_FONT_PATH,
                                              LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                              32,
                                              LV_FREETYPE_FONT_STYLE_NORMAL);
    TEST_ASSERT_NOT_NULL(font_devanagari);
    lv_freetype_font_set_use_harfbuzz(font_devanagari, true);
}

void tearDown(void)
{
    lv_obj_clean(lv_screen_active());
    if(font_devanagari) {
        lv_freetype_font_delete(font_devanagari);
        font_devanagari = NULL;
    }
}

void test_freetype_font_is_harfbuzz(void)
{
    TEST_ASSERT_TRUE(lv_freetype_is_harfbuzz_font(font_devanagari));

    /*The per-font toggle disables and re-enables shaping*/
    lv_freetype_font_set_use_harfbuzz(font_devanagari, false);
    TEST_ASSERT_FALSE(lv_freetype_is_harfbuzz_font(font_devanagari));
    lv_freetype_font_set_use_harfbuzz(font_devanagari, true);
    TEST_ASSERT_TRUE(lv_freetype_is_harfbuzz_font(font_devanagari));

    /*Built-in bitmap fonts are never HarfBuzz fonts*/
    TEST_ASSERT_FALSE(lv_freetype_is_harfbuzz_font(&lv_font_montserrat_14));
}

void test_freetype_harfbuzz_shapes_conjuncts(void)
{
    /*"ksha": KA + VIRAMA + SSA (3 codepoints) fuses into fewer glyphs than
     *codepoints when shaping works. Without shaping each codepoint maps to
     *its own glyph.*/
    const char * ksha = "क्ष";
    lv_hb_shaped_text_t * shaped = lv_hb_shape_text(font_devanagari, ksha, strlen(ksha), LV_BASE_DIR_AUTO);
    TEST_ASSERT_NOT_NULL(shaped);
    TEST_ASSERT_LESS_THAN_UINT32(3, shaped->count);
    lv_hb_shaped_text_destroy(shaped);
}

void test_freetype_harfbuzz_cluster_mapping(void)
{
    /*"ki": KA + vowel sign I. The matra is displayed before the consonant
     *but both glyphs must keep the cluster of the syllable start so that
     *cursor positioning maps back to the right character.*/
    const char * ki = "कि";
    lv_hb_shaped_text_t * shaped = lv_hb_shape_text(font_devanagari, ki, strlen(ki), LV_BASE_DIR_AUTO);
    TEST_ASSERT_NOT_NULL(shaped);
    TEST_ASSERT_GREATER_THAN_UINT32(0, shaped->count);
    for(uint32_t i = 0; i < shaped->count; i++) {
        TEST_ASSERT_EQUAL_UINT32(0, shaped->glyphs[i].cluster);
    }
    lv_hb_shaped_text_destroy(shaped);
}

static lv_obj_t * add_label(lv_obj_t * parent, const char * text)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font_devanagari, 0);
    lv_label_set_text(label, text);
    return label;
}

static void create_devanagari_labels(void)
{
    lv_obj_t * cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont, lv_pct(90), LV_SIZE_CONTENT);
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(cont, 8, 0);

    add_label(cont, "नमस्ते दुनिया");           /*Hello World*/
    add_label(cont, "क्ष त्र ज्ञ श्र");          /*conjuncts*/
    add_label(cont, "कि की कु कू के कै को कौ");  /*matra reordering*/
    add_label(cont, "स्त्री विद्या राष्ट्र");     /*half-forms and reph*/
    add_label(cont, "हिन्दी");                  /*the string from issue #10370*/
    add_label(cont, "मराठी भाषा ळ");            /*Marathi, incl. retroflex LLA*/
}

/*Shaping turns a string into positioned glyphs. One input character can
 *produce several glyphs, and several characters can fuse into one glyph, so
 *glyphs are grouped by "cluster": the character they came from. Spacing
 *belongs between clusters, never between a glyph and the marks attached to
 *it. This mirrors the sum the draw path performs.*/
static int32_t shaped_advance_sum(const char * txt, int32_t letter_space)
{
    lv_hb_shaped_text_t * shaped = lv_hb_shape_text(font_devanagari, txt, lv_strlen(txt), LV_BASE_DIR_AUTO);
    TEST_ASSERT_NOT_NULL(shaped);

    int32_t sum = 0;
    for(uint32_t i = 0; i < shaped->count; i++) {
        bool cluster_end = (i + 1 >= shaped->count) || (shaped->glyphs[i + 1].cluster != shaped->glyphs[i].cluster);
        sum += shaped->glyphs[i].x_advance + (cluster_end ? letter_space : 0);
    }
    if(sum > 0) sum -= letter_space;

    lv_hb_shaped_text_destroy(shaped);
    return sum;
}

void test_freetype_harfbuzz_letter_space_per_cluster(void)
{
    /*"Hindi" in Devanagari. It has more glyphs than clusters because the
     *vowel sign is a separate glyph attached to its consonant. Raising
     *letter_space must widen the line once per cluster gap, not once per
     *glyph, or the vowel sign drifts away from the letter it belongs to.*/
    const char * txt = "\xe0\xa4\xb9\xe0\xa4\xbf\xe0\xa4\xa8\xe0\xa5\x8d\xe0\xa4\xa6\xe0\xa5\x80";

    lv_hb_shaped_text_t * shaped = lv_hb_shape_text(font_devanagari, txt, lv_strlen(txt), LV_BASE_DIR_AUTO);
    TEST_ASSERT_NOT_NULL(shaped);
    uint32_t glyphs = shaped->count;
    uint32_t clusters = 0;
    for(uint32_t i = 0; i < shaped->count; i++) {
        if(i == 0 || shaped->glyphs[i].cluster != shaped->glyphs[i - 1].cluster) clusters++;
    }
    lv_hb_shaped_text_destroy(shaped);

    TEST_ASSERT_GREATER_THAN_UINT32(clusters, glyphs);

    int32_t w0 = shaped_advance_sum(txt, 0);
    int32_t w4 = shaped_advance_sum(txt, 4);
    TEST_ASSERT_EQUAL_INT32(w0 + (int32_t)(clusters - 1) * 4, w4);
}

void test_freetype_harfbuzz_label_width_matches_advances(void)
{
    /*The width a label reports is used to centre and right-align it. If it
     *disagrees with what the draw path advances, aligned text is drawn off
     *its own box. "Marathi" in Devanagari, with spacing on.*/
    const char * txt = "\xe0\xa4\xae\xe0\xa4\xb0\xe0\xa4\xbe\xe0\xa4\xa0\xe0\xa5\x80";

    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label, font_devanagari, 0);
    lv_obj_set_style_text_letter_space(label, 4, 0);
    lv_label_set_text(label, txt);
    lv_obj_update_layout(label);

    TEST_ASSERT_EQUAL_INT32(shaped_advance_sum(txt, 4), lv_obj_get_width(label));
}

void test_freetype_harfbuzz_hit_test_round_trip(void)
{
    /*Where a character is drawn and where a click on it lands are computed by
     *separate code, and they have to agree. A cluster is a letter plus the
     *marks that attach to it; it is one unit on screen, so a click anywhere
     *inside it selects the character the cluster starts at. "Marathi" below
     *has two such clusters, each a consonant with a vowel sign.*/
    const char * txt = "\xe0\xa4\xae\xe0\xa4\xb0\xe0\xa4\xbe\xe0\xa4\xa0\xe0\xa5\x80";

    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label, font_devanagari, 0);
    lv_obj_set_style_text_letter_space(label, 4, 0);
    lv_label_set_text(label, txt);
    lv_obj_update_layout(label);

    int32_t prev_x = -1;
    for(uint32_t ch = 0; ch < 5; ch++) {
        lv_point_t pos;
        lv_label_get_letter_pos(label, ch, &pos);

        /*Characters are laid out left to right, so positions never go back*/
        TEST_ASSERT_GREATER_OR_EQUAL_INT32(prev_x, pos.x);
        prev_x = pos.x;

        lv_point_t click = pos;
        click.x += 1;
        uint32_t hit = lv_label_get_letter_on(label, &click, false);

        /*The hit is this character or the start of the cluster it joined*/
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(ch, hit);

        /*Clicking the hit character must return the same character again*/
        lv_point_t again;
        lv_label_get_letter_pos(label, hit, &again);
        again.x += 1;
        TEST_ASSERT_EQUAL_UINT32(hit, lv_label_get_letter_on(label, &again, false));
    }

    /*The first character is always reachable at the start of the line*/
    lv_point_t origin;
    lv_label_get_letter_pos(label, 0, &origin);
    TEST_ASSERT_EQUAL_INT32(0, origin.x);
}

void test_freetype_harfbuzz_recolor_is_not_shaped(void)
{
    /*Recolor markers such as "#ff0000 " are instructions, not text. The
     *shaping path cannot strip them, so a line containing one is rendered
     *by the character path instead. If it were shaped, the markers would
     *appear as glyphs and make the label wider.*/
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label, font_devanagari, 0);
    lv_label_set_recolor(label, true);
    lv_label_set_text(label, "#ff0000 \xe0\xa4\xae\xe0\xa4\xb0#");
    lv_obj_update_layout(label);

    lv_obj_t * plain = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(plain, font_devanagari, 0);
    lv_label_set_text(plain, "\xe0\xa4\xae\xe0\xa4\xb0");
    lv_obj_update_layout(plain);

    TEST_ASSERT_EQUAL_INT32(lv_obj_get_width(plain), lv_obj_get_width(label));
}

void test_freetype_harfbuzz_off_by_default(void)
{
    /*Shaping changes how every glyph is placed, so a font only gets it when
     *asked. This keeps existing FreeType text rendering untouched.*/
    lv_font_t * plain = lv_freetype_font_create(DEVANAGARI_FONT_PATH,
                                                LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                                32,
                                                LV_FREETYPE_FONT_STYLE_NORMAL);
    TEST_ASSERT_NOT_NULL(plain);
    TEST_ASSERT_FALSE(lv_freetype_is_harfbuzz_font(plain));
    lv_freetype_font_delete(plain);
}

void test_freetype_harfbuzz_render_devanagari(void)
{
    create_devanagari_labels();
    TEST_ASSERT_EQUAL_SCREENSHOT("libs/freetype_harfbuzz_devanagari" EXT_NAME);
}

void test_freetype_harfbuzz_render_disabled(void)
{
    /*With shaping disabled the same text must still render (unshaped),
     *using the character-by-character path*/
    lv_freetype_font_set_use_harfbuzz(font_devanagari, false);
    create_devanagari_labels();
    TEST_ASSERT_EQUAL_SCREENSHOT("libs/freetype_harfbuzz_disabled" EXT_NAME);
}

#else /*LV_USE_FREETYPE && LV_USE_HARFBUZZ*/

void setUp(void)
{
}

void tearDown(void)
{
}

void test_freetype_font_is_harfbuzz(void)
{
}

void test_freetype_harfbuzz_shapes_conjuncts(void)
{
}

void test_freetype_harfbuzz_cluster_mapping(void)
{
}

void test_freetype_harfbuzz_letter_space_per_cluster(void)
{
}

void test_freetype_harfbuzz_label_width_matches_advances(void)
{
}

void test_freetype_harfbuzz_hit_test_round_trip(void)
{
}

void test_freetype_harfbuzz_recolor_is_not_shaped(void)
{
}

void test_freetype_harfbuzz_off_by_default(void)
{
}

void test_freetype_harfbuzz_render_devanagari(void)
{
}

void test_freetype_harfbuzz_render_disabled(void)
{
}

#endif /*LV_USE_FREETYPE && LV_USE_HARFBUZZ*/

#endif /*LV_BUILD_TEST*/
