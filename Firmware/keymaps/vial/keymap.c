#include QMK_KEYBOARD_H
#include <stdio.h>
#include <string.h>
#include <qp.h>

#include "assets/idle_240x320_cropped.qgf.h"
#include "assets/segoeui.qff.h"
#include "assets/bongocat_base.qgf.h"

enum layers {
    _BASE,
    _LOWER,
    _L2,
    _L3
};

typedef enum {
    LCD_MODE_IDLE,
    LCD_MODE_BASE_STATS,
    LCD_MODE_BONGO
} lcd_mode_t;

static painter_device_t       tft;
static painter_font_handle_t  font_main;
static painter_image_handle_t img_idle;
static painter_image_handle_t img_bongo;

static deferred_token bongo_anim;
static bool           bongo_running = false;

static deferred_token idle_anim;
static bool           idle_running = false;

static uint32_t last_typing_ms = 0;

#define BONGO_STOP_MS     500
#define LCD_IDLE_TIMEOUT  60000
#define WPM_HISTORY_LEN   64
#define CHART_UPDATE_MS   250
#define CHART_WPM_MAX     120

static lcd_mode_t current_lcd_mode = LCD_MODE_IDLE;
static uint8_t    current_layer_id = _BASE;
static uint32_t   last_activity_ms = 0;
static uint32_t   last_chart_ms    = 0;
static uint8_t    last_mods        = 0;

static uint8_t wpm_history[WPM_HISTORY_LEN] = {0};

const key_override_t *key_overrides[] = {
    NULL
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = {
        { LCA(KC_TAB), KC_MINS, KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_NO },
        { KC_TAB,      KC_LBRC, KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_MS_U },
        { KC_ENT,      KC_NUHS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_MS_L },
        { KC_LSFT,     KC_NUBS, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_BTN1 },
        { _______,     _______, _______, _______, KC_LGUI, KC_LCTL, KC_PGUP, KC_MS_R },
        { _______,     _______, _______, _______, _______, _______, KC_SPC,  KC_MS_D },

        { KC_MUTE,     KC_EQL,  KC_0,    KC_9,    KC_8,    KC_7,    KC_6,    KC_NO },
        { KC_ESC,      KC_RBRC, KC_P,    KC_O,    KC_I,    KC_U,    KC_Y,    KC_UP },
        { KC_HOME,     KC_QUOT, KC_SCLN, KC_L,    KC_K,    KC_J,    KC_H,    KC_LEFT },
        { KC_END,      KC_SLSH, KC_DOT,  KC_COMM, KC_M,    KC_N,    KC_B,    KC_BTN2 },
        { _______,     _______, _______, _______, KC_LALT, MO(_LOWER), KC_PGDN, KC_RGHT },
        { _______,     _______, _______, _______, _______, _______, KC_BSPC, KC_DOWN }
    },

    [_LOWER] = {
        { _______, _______, _______, _______, _______, _______, _______, KC_NO },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },

        { _______, _______, _______, _______, _______, _______, _______, KC_NO },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ }
    },

    [_L2] = {
        { _______, _______, _______, _______, _______, _______, _______, KC_NO },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },

        { _______, _______, _______, _______, _______, _______, _______, KC_NO },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ }
    },

    [_L3] = {
        { _______, _______, _______, _______, _______, _______, _______, KC_NO },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },

        { _______, _______, _______, _______, _______, _______, _______, KC_NO },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______, _______ }
    }
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_BASE]  = {
        ENCODER_CCW_CW(KC_LEFT, KC_RIGHT),
        ENCODER_CCW_CW(KC_VOLU, KC_VOLD)
    },
    [_LOWER] = {
        ENCODER_CCW_CW(KC_BRIU, KC_BRID),
        ENCODER_CCW_CW(KC_NO,   KC_NO)
    },
    [_L2] = {
        ENCODER_CCW_CW(KC_NO, KC_NO),
        ENCODER_CCW_CW(KC_NO, KC_NO)
    },
    [_L3] = {
        ENCODER_CCW_CW(KC_NO, KC_NO),
        ENCODER_CCW_CW(KC_NO, KC_NO)
    }
};
#endif

static const char *get_layer_name(uint8_t layer) {
    switch (layer) {
        case _BASE:  return "BASE";
        case _LOWER: return "LAYER 1";
        case _L2:    return "LAYER 2";
        case _L3:    return "LAYER 3";
        default:     return "?";
    }
}

static uint8_t get_current_mod_state(void) {
    return get_mods() | get_oneshot_mods() | get_weak_mods();
}

static void note_activity(void) {
    last_activity_ms = timer_read32();
}

static bool lcd_should_idle(void) {
    return timer_elapsed32(last_activity_ms) >= LCD_IDLE_TIMEOUT;
}

static void clear_screen(void) {
    qp_rect(tft, 0, 0, 239, 319, 0, 0, 0, true);
    qp_flush(tft);
}

static void push_wpm_sample(uint8_t wpm) {
    memmove(&wpm_history[0], &wpm_history[1], WPM_HISTORY_LEN - 1);
    wpm_history[WPM_HISTORY_LEN - 1] = wpm;
}

static void stop_idle_animation(void) {
    if (idle_running) {
        qp_stop_animation(idle_anim);
        idle_running = false;
    }
}

static void stop_bongo_animation(void) {
    if (bongo_running) {
        qp_stop_animation(bongo_anim);
        bongo_running = false;
    }
}

static void render_idle(void) {
    stop_bongo_animation();
    stop_idle_animation();

    clear_screen();

    if (img_idle) {
        idle_anim = qp_animate(tft, 0, 0, img_idle);
        idle_running = true;
    }

    current_lcd_mode = LCD_MODE_IDLE;
}

static void draw_wpm_chart(void) {
    const uint16_t x0 = 10;
    const uint16_t y0 = 80;
    const uint16_t x1 = 230;
    const uint16_t y1 = 250;
    const uint16_t w  = x1 - x0;
    const uint16_t h  = y1 - y0;

    qp_rect(tft, x0, y0, x1, y1, 0, 0, 0, true);
    qp_line(tft, x0, y1, x1, y1, 0, 0, 255);
    qp_line(tft, x0, y0, x0, y1, 0, 0, 255);

    for (uint8_t i = 1; i < WPM_HISTORY_LEN; i++) {
        uint8_t v0 = wpm_history[i - 1];
        uint8_t v1 = wpm_history[i];

        if (v0 > CHART_WPM_MAX) v0 = CHART_WPM_MAX;
        if (v1 > CHART_WPM_MAX) v1 = CHART_WPM_MAX;

        uint16_t px0 = x0 + ((uint32_t)(i - 1) * w) / (WPM_HISTORY_LEN - 1);
        uint16_t px1 = x0 + ((uint32_t)i * w) / (WPM_HISTORY_LEN - 1);

        uint16_t py0 = y1 - ((uint32_t)v0 * h) / CHART_WPM_MAX;
        uint16_t py1 = y1 - ((uint32_t)v1 * h) / CHART_WPM_MAX;

        qp_line(tft, px0, py0, px1, py1, 0, 0, 255);
    }
}

static void render_left_dashboard(bool full_redraw) {
    bool needs_full_redraw = full_redraw || idle_running || current_lcd_mode == LCD_MODE_IDLE;

    if (needs_full_redraw) {
        stop_idle_animation();
        clear_screen();
    }

    char wpm_buf[16];
    char mods_buf[8];

    uint8_t mods = get_current_mod_state();

    snprintf(wpm_buf, sizeof(wpm_buf), "%3u", get_current_wpm());

    snprintf(
        mods_buf,
        sizeof(mods_buf),
        "%c%c%c%c",
        (mods & MOD_MASK_SHIFT) ? 'S' : '-',
        (mods & MOD_MASK_CTRL)  ? 'C' : '-',
        (mods & MOD_MASK_ALT)   ? 'A' : '-',
        (mods & MOD_MASK_GUI)   ? 'G' : '-'
    );

    if (needs_full_redraw) {
        qp_drawtext_recolor(tft, 10, 12,  font_main, get_layer_name(current_layer_id), 0, 0, 255, 0, 0, 0);
        qp_drawtext_recolor(tft, 10, 40,  font_main, "WPM",                        0, 0, 255, 0, 0, 0);
        qp_drawtext_recolor(tft, 10, 292, font_main, "MODS",                       0, 0, 255, 0, 0, 0);
    } else {
        qp_rect(tft, 10, 8,   230, 32,  0, 0, 0, true);
        qp_rect(tft, 55, 34,  150, 62,  0, 0, 0, true);
        qp_rect(tft, 80, 286, 230, 315, 0, 0, 0, true);
    }

    qp_drawtext_recolor(tft, 10, 12,  font_main, get_layer_name(current_layer_id), 0, 0, 255, 0, 0, 0);
    qp_drawtext_recolor(tft, 55, 40,  font_main, wpm_buf,                          0, 0, 255, 0, 0, 0);
    qp_drawtext_recolor(tft, 80, 292, font_main, mods_buf,                         0, 0, 255, 0, 0, 0);

    draw_wpm_chart();

    qp_flush(tft);
    current_lcd_mode = LCD_MODE_BASE_STATS;
}

static void render_right_bongo(bool full_redraw) {
    if (idle_running || current_lcd_mode == LCD_MODE_IDLE || full_redraw) {
        stop_idle_animation();
        clear_screen();
    }

    if (!full_redraw && current_lcd_mode == LCD_MODE_BONGO) {
        return;
    }

    if (img_bongo) {
        qp_drawimage(tft, 0, 0, img_bongo);
    } else {
        qp_drawtext_recolor(tft, 10, 20, font_main, "BONGO IMG FAIL", 0, 0, 255, 0, 0, 0);
    }

    qp_flush(tft);
    current_lcd_mode = LCD_MODE_BONGO;
}

static void render_current_screen(bool full_redraw) {
    if (lcd_should_idle()) {
        if (current_lcd_mode != LCD_MODE_IDLE || full_redraw) {
            render_idle();
        }
        return;
    }

    if (is_keyboard_master()) {
        render_right_bongo(full_redraw);
    } else {
        render_left_dashboard(full_redraw);
    }
}

void keyboard_post_init_user(void) {
    note_activity();

    setPinInputHigh(GP15);

    tft = qp_ili9341_make_spi_device(240, 320, TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN, 4, 0);
    qp_init(tft, QP_ROTATION_0);

    font_main = qp_load_font_mem(font_segoeui);
    img_idle  = qp_load_image_mem(gfx_idle_240x320_cropped);
    img_bongo = qp_load_image_mem(gfx_bongocat_base);

    current_layer_id = get_highest_layer(layer_state);
    last_mods        = get_current_mod_state();

    render_current_screen(true);
}

layer_state_t layer_state_set_user(layer_state_t state) {
    current_layer_id = get_highest_layer(state);
    note_activity();
    render_current_screen(true);
    return state;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        note_activity();
        last_typing_ms = timer_read32();

        stop_idle_animation();

        if (current_lcd_mode == LCD_MODE_IDLE) {
            current_layer_id = get_highest_layer(layer_state);
            render_current_screen(true);
        }

        if (is_keyboard_master() && img_bongo && !bongo_running) {
            clear_screen();
            bongo_anim = qp_animate(tft, 0, 0, img_bongo);
            bongo_running = true;
            current_lcd_mode = LCD_MODE_BONGO;
        }
    }

    return true;
}

bool encoder_update_user(uint8_t index, bool clockwise) {
    note_activity();
    stop_idle_animation();

    if (current_lcd_mode == LCD_MODE_IDLE) {
        current_layer_id = get_highest_layer(layer_state);
        render_current_screen(true);
    }

    return false;
}

static uint32_t boot_btn_timer = 0;

void housekeeping_task_user(void) {
    if (!lcd_should_idle() && current_lcd_mode == LCD_MODE_IDLE) {
        stop_idle_animation();
        render_current_screen(true);
    }

    if (!readPin(GP15)) {
        if (boot_btn_timer == 0) {
            boot_btn_timer = timer_read32();
        }

        if (timer_elapsed32(boot_btn_timer) > 500) {
            reset_keyboard();
        }
    } else {
        boot_btn_timer = 0;
    }

    uint8_t new_layer_id = get_highest_layer(layer_state);

    if (new_layer_id != current_layer_id) {
        current_layer_id = new_layer_id;
        note_activity();
        render_current_screen(true);
        return;
    }

    uint8_t wpm = get_current_wpm();

    if (wpm > 0) {
        note_activity();
    }

    if (lcd_should_idle()) {
        if (current_lcd_mode != LCD_MODE_IDLE) {
            render_idle();
        }
        return;
    }

    if (is_keyboard_master()) {
        if (bongo_running && timer_elapsed32(last_typing_ms) > BONGO_STOP_MS) {
            stop_bongo_animation();
            clear_screen();

            if (img_bongo) {
                qp_drawimage(tft, 0, 0, img_bongo);
            }

            qp_flush(tft);
            current_lcd_mode = LCD_MODE_BONGO;
        }

        return;
    }

    uint8_t mods = get_current_mod_state();

    if (mods != last_mods) {
        last_mods = mods;
        render_left_dashboard(false);
        return;
    }

    if (timer_elapsed32(last_chart_ms) >= CHART_UPDATE_MS) {
        last_chart_ms = timer_read32();
        push_wpm_sample(wpm);
        render_left_dashboard(false);
    }
}