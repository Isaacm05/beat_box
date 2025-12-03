#include "ui.h"
#include "bpm.h"
#include "led_matrix.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <time.h>

#define TARGET_FPS 60
#define FRAME_TIME_US (1000000 / TARGET_FPS)
#define COLOR_CHANGE_INTERVAL_US 1000000 // 1 second
#define DEBOUNCE_US 15000                // 15 ms debounce
#define LONG_PRESS_US 400000             // 400 ms for long-press

// Global UI vertical offset (moves everything down)
#define UI_Y_OFFSET 5

// button control vars
#define PIN_SELECT 31

static const uint BUTTON_PINS[5] = {PIN_LEFT, PIN_RIGHT, PIN_UP, PIN_DOWN, PIN_SELECT};

enum Button {
    BUTTON_LEFT = 0,
    BUTTON_RIGHT = 1,
    BUTTON_UP = 2,
    BUTTON_DOWN = 3,
    BUTTON_SELECT = 4
};

static volatile bool button_short_press[5] = {0};
static volatile bool button_long_press[5] = {0};

static volatile bool button_raw_event[5] = {0};
static bool last_raw_state[5] = {0};
static bool stable_state[5] = {0};
static uint64_t last_change_time[5] = {0};
static uint64_t press_start_time[5] = {0};

// UI menu vars
static int menu_index = 0;   // scrolls through tracks
static int num_tracks = 4;   // fixed at 4 tracks
static int active_track = 0; // which track is currently active (for editing)

// Cursor position system
typedef enum { CURSOR_BPM, CURSOR_PLAY, CURSOR_TRACK_LABEL, CURSOR_BEAT } CursorMode;

static CursorMode cursor_mode = CURSOR_TRACK_LABEL;
static int cursor_track = 0; // which track row (0-3)
static int cursor_beat = 0;  // which beat column (0-8 for 9 beats)

// Previous cursor position for selective redraw
static CursorMode prev_cursor_mode = CURSOR_TRACK_LABEL;
static int prev_cursor_track = 0;
static int prev_cursor_beat = 0;
static int prev_active_track = 0;
static int prev_preset_cursor = 0;

#define NUM_BEATS 8
#define NUM_PRESETS 4
static const char* preset_names[NUM_PRESETS] = {"SIN", "TRI", "SQR", "NOI"};

// Track presets
static int track_preset[4] = {-1, -1, -1, -1};

// Preset selection mode
static bool preset_selection_mode = false;
static bool prev_preset_selection_mode = false;
static int preset_selection_track = 0;
static int preset_cursor = 0;

// Beat state for each track (true = filled/active)
static bool beat_state[4][NUM_BEATS] = {0};

// Track bpm for change detect
static int prev_bpm = 120;

// Playback state
static bool is_playing = false;
static int current_beat = 0;
static uint64_t last_beat_time = 0;

// Draw waveform preset icons
static void draw_waveform(int x, int y, int preset_type, uint8_t r, uint8_t g, uint8_t b) {
    switch (preset_type) {
    case 0: // Sine wave
        led_matrix_set_pixel(x, y + 2, r, g, b);
        led_matrix_set_pixel(x + 1, y + 1, r, g, b);
        led_matrix_set_pixel(x + 2, y, r, g, b);
        led_matrix_set_pixel(x + 3, y + 1, r, g, b);
        led_matrix_set_pixel(x + 4, y + 2, r, g, b);
        led_matrix_set_pixel(x + 5, y + 3, r, g, b);
        led_matrix_set_pixel(x + 6, y + 4, r, g, b);
        led_matrix_set_pixel(x + 7, y + 3, r, g, b);
        led_matrix_set_pixel(x + 8, y + 2, r, g, b);
        break;
    case 1: // Triangle wave
        led_matrix_set_pixel(x, y + 4, r, g, b);
        led_matrix_set_pixel(x + 1, y + 3, r, g, b);
        led_matrix_set_pixel(x + 2, y + 2, r, g, b);
        led_matrix_set_pixel(x + 3, y + 1, r, g, b);
        led_matrix_set_pixel(x + 4, y, r, g, b);
        led_matrix_set_pixel(x + 5, y + 1, r, g, b);
        led_matrix_set_pixel(x + 6, y + 2, r, g, b);
        led_matrix_set_pixel(x + 7, y + 3, r, g, b);
        led_matrix_set_pixel(x + 8, y + 4, r, g, b);
        break;
    case 2: // Square wave
        led_matrix_set_pixel(x, y, r, g, b);
        led_matrix_set_pixel(x + 1, y, r, g, b);
        led_matrix_set_pixel(x + 2, y, r, g, b);
        led_matrix_set_pixel(x + 3, y, r, g, b);
        led_matrix_set_pixel(x + 4, y + 4, r, g, b);
        led_matrix_set_pixel(x + 5, y + 4, r, g, b);
        led_matrix_set_pixel(x + 6, y + 4, r, g, b);
        led_matrix_set_pixel(x + 7, y + 4, r, g, b);
        led_matrix_set_pixel(x + 8, y, r, g, b);
        break;
    case 3: // Noise
        led_matrix_set_pixel(x, y + 1, r, g, b);
        led_matrix_set_pixel(x + 1, y + 3, r, g, b);
        led_matrix_set_pixel(x + 2, y, r, g, b);
        led_matrix_set_pixel(x + 3, y + 4, r, g, b);
        led_matrix_set_pixel(x + 4, y + 2, r, g, b);
        led_matrix_set_pixel(x + 5, y, r, g, b);
        led_matrix_set_pixel(x + 6, y + 3, r, g, b);
        led_matrix_set_pixel(x + 7, y + 1, r, g, b);
        led_matrix_set_pixel(x + 8, y + 4, r, g, b);
        break;
    }
}

// button initialization
static void buttons_init(void) {
    for (int i = 0; i < 5; i++) {
        gpio_init(BUTTON_PINS[i]);
        gpio_set_dir(BUTTON_PINS[i], GPIO_IN);
        gpio_pull_up(BUTTON_PINS[i]); // active LOW
    }
}

// button interrupt service routine
static void button_isr(uint gpio, uint32_t events) {
    for (int i = 0; i < 5; i++) {
        if (gpio == BUTTON_PINS[i]) {
            button_raw_event[i] = true;
            last_change_time[i] = time_us_64();
        }
    }
}

// process button state with debounce
static void process_button(int b) {
    uint64_t now = time_us_64();

    // Raw event from ISR?
    if (button_raw_event[b]) {
        button_raw_event[b] = false;
        // actual debounce happens below
    }

    bool raw = (gpio_get(BUTTON_PINS[b]) == 0);

    // Detect changes in raw signal
    if (raw != last_raw_state[b]) {
        last_raw_state[b] = raw;
        last_change_time[b] = now;
    }

    // Not yet debounced
    if (now - last_change_time[b] < DEBOUNCE_US)
        return;

    // State is debounced & stable
    if (raw != stable_state[b]) {
        stable_state[b] = raw;

        if (stable_state[b]) {
            // PRESSED
            press_start_time[b] = now;
        } else {
            uint64_t press_len = now - press_start_time[b];

            if (press_len >= LONG_PRESS_US) {
                button_long_press[b] = true;
            } else {
                button_short_press[b] = true;
            }
        }
    }
}

// Redraw a single beat cell
static void redraw_beat_cell(int track, int beat) {
    int grid_x = 14;
    int grid_y = 20 + UI_Y_OFFSET;
    int cell_w = 4;
    int cell_h = 4;
    int spacing = 2;
    int row_spacing = 5;

    int x = grid_x + beat * (cell_w + spacing);
    int y = grid_y + track * (cell_h + row_spacing);

    bool is_cursor = (cursor_mode == CURSOR_BEAT && cursor_track == track && cursor_beat == beat);
    bool is_filled = beat_state[track][beat];
    bool is_current_beat = (is_playing && beat == current_beat);

    // Outline: blue for cursor, yellow for playback position, gray otherwise
    if (is_cursor) {
        led_matrix_draw_rect(x, y, cell_w, cell_h, 0, 0, 255);
    } else if (is_current_beat) {
        led_matrix_draw_rect(x, y, cell_w, cell_h, 255, 255, 0);
    } else {
        led_matrix_draw_rect(x, y, cell_w, cell_h, 200, 200, 200);
    }

    if (is_filled) {
        led_matrix_fill_rect(x + 1, y + 1, cell_w - 2, cell_h - 2, 255, 255, 0);
    } else {
        led_matrix_fill_rect(x + 1, y + 1, cell_w - 2, cell_h - 2, 0, 0, 0);
    }
}

// Redraw track label
static void redraw_track_label(int track) {
    int box_x = 5;
    int box_y_start = 19 + UI_Y_OFFSET;
    int box_w = 7;
    int box_h = 7;
    int spacing = 2;

    const char* labels[4] = {"1", "2", "3", "4"};
    int y = box_y_start + track * (box_h + spacing);

    bool is_cursor = (cursor_mode == CURSOR_TRACK_LABEL && cursor_track == track);
    bool is_active = (track == active_track);
    bool has_preset = (track_preset[track] != -1);

    if (is_cursor) {
        led_matrix_draw_rect(box_x - 1, y - 1, box_w + 2, box_h + 2, 0, 0, 255);
    } else {
        // Clear cursor outline
        led_matrix_draw_rect(box_x - 1, y - 1, box_w + 2, box_h + 2, 0, 0, 0);
    }

    if (is_active && has_preset) {
        // Active track with preset: red background, white text
        led_matrix_fill_rect(box_x, y, box_w, box_h, 255, 0, 0);
        led_matrix_draw_text(box_x + 2, y + 1, labels[track], 255, 255, 255);
    } else if (has_preset) {
        // Has preset but not active: white background, black text
        led_matrix_fill_rect(box_x, y, box_w, box_h, 255, 255, 255);
        led_matrix_draw_text(box_x + 2, y + 1, labels[track], 0, 0, 0);
    } else {
        // No preset: black background, white text
        led_matrix_fill_rect(box_x, y, box_w, box_h, 0, 0, 0);
        led_matrix_draw_text(box_x + 2, y + 1, labels[track], 255, 255, 255);
    }
}

// Draw BPM number (3 digits)
static void draw_bpm_number(int x, int y, int bpm, uint8_t r, uint8_t g, uint8_t b) {
    char bpm_str[4];
    bpm_str[0] = '0' + (bpm / 100);
    bpm_str[1] = '0' + ((bpm / 10) % 10);
    bpm_str[2] = '0' + (bpm % 10);
    bpm_str[3] = '\0';
    led_matrix_draw_text(x, y, bpm_str, r, g, b);
}

// Redraw BPM box cursor
static void redraw_bpm_cursor() {
    int bpm_x = 4;
    int bpm_y = 9 + UI_Y_OFFSET;
    int bpm_w = 13;
    int bpm_h = 7;

    bool is_flashing = bpm_is_flashing();

    if (cursor_mode == CURSOR_BPM) {
        led_matrix_draw_rect(bpm_x - 1, bpm_y - 1, bpm_w + 2, bpm_h + 2, 0, 0, 255);
    } else {
        led_matrix_draw_rect(bpm_x - 1, bpm_y - 1, bpm_w + 2, bpm_h + 2, 0, 0, 0);
    }

    // Flash white on tap, otherwise black
    if (is_flashing) {
        led_matrix_fill_rect(bpm_x, bpm_y, bpm_w, bpm_h, 0, 0, 0);
        draw_bpm_number(bpm_x + 1, bpm_y + 1, bpm_get(), 255, 0, 0);
    } else {
        led_matrix_fill_rect(bpm_x, bpm_y, bpm_w, bpm_h, 0, 0, 0);
        draw_bpm_number(bpm_x + 1, bpm_y + 1, bpm_get(), 0, 255, 255);
    }
}

// Redraw PLAY/STOP box cursor
static void redraw_play_cursor() {
    int play_x = 41;
    int play_y = 9 + UI_Y_OFFSET;
    int play_w = 17;
    int play_h = 7;

    if (cursor_mode == CURSOR_PLAY) {
        led_matrix_draw_rect(play_x - 1, play_y - 1, play_w + 2, play_h + 2, 0, 0, 255);
    } else {
        led_matrix_draw_rect(play_x - 1, play_y - 1, play_w + 2, play_h + 2, 0, 0, 0);
    }
    led_matrix_fill_rect(play_x, play_y, play_w, play_h, 0, 0, 0);

    if (is_playing) {
        led_matrix_draw_text(play_x + 1, play_y + 1, "STOP", 255, 0, 0);
    } else {
        led_matrix_draw_text(play_x + 1, play_y + 1, "PLAY", 0, 255, 0);
    }
}

// Draw or clear presets area
static void draw_presets_area() {
    int preset_y = 56 + UI_Y_OFFSET;
    int preset_x_start = 4;
    int preset_spacing = 14;

    if (!preset_selection_mode) {
        // Clear presets area
        led_matrix_fill_rect(preset_x_start, preset_y, 60, 8, 0, 0, 0);
        return;
    }

    for (int i = 0; i < NUM_PRESETS; i++) {
        int x = preset_x_start + i * preset_spacing;
        bool is_preset_cursor = (i == preset_cursor);

        uint8_t r = (is_preset_cursor) ? 0 : 80;
        uint8_t g = (is_preset_cursor) ? 0 : 80;
        uint8_t b = (is_preset_cursor) ? 255 : 80;

        draw_waveform(x, preset_y, i, r, g, b);
    }
}

// draw grid for tracks
static void draw_grid(int num_tracks) {
    int grid_x = 14;               // left side of grid
    int grid_y = 20 + UI_Y_OFFSET; // below title
    int cell_w = 4;                // square size
    int cell_h = 4;                // square size
    int spacing = 2;
    int row_spacing = 5; // vertical spacing between rows

    // grid cells
    for (int row = 0; row < num_tracks; row++) {
        for (int col = 0; col < NUM_BEATS; col++) {

            int x = grid_x + col * (cell_w + spacing);
            int y = grid_y + row * (cell_h + row_spacing);

            bool is_cursor =
                (cursor_mode == CURSOR_BEAT && cursor_track == row && cursor_beat == col);
            bool is_filled = beat_state[row][col];

            if (is_cursor) {
                led_matrix_draw_rect(x, y, cell_w, cell_h, 0, 0, 255);
            } else {
                led_matrix_draw_rect(x, y, cell_w, cell_h, 200, 200, 200);
            }

            if (is_filled) {
                led_matrix_fill_rect(x + 1, y + 1, cell_w - 2, cell_h - 2, 255, 255, 0);
            } else {
                led_matrix_fill_rect(x + 1, y + 1, cell_w - 2, cell_h - 2, 0, 0, 0);
            }
        }
    }
}

// draw UI menu
static void draw_menu() {
    led_matrix_clear();

    // title
    const char* title = "BEAT BOX";
    int title_len = 8;
    int char_w = 4;
    int title_px = title_len * char_w;
    int title_x = (64 - title_px) / 2;

    led_matrix_draw_text(title_x, 2 + UI_Y_OFFSET, title, 255, 255, 0);

    // BPM box (left side)
    int bpm_x = 4;
    int bpm_y = 9 + UI_Y_OFFSET;
    int bpm_w = 13;
    int bpm_h = 7;

    if (cursor_mode == CURSOR_BPM) {
        led_matrix_draw_rect(bpm_x - 1, bpm_y - 1, bpm_w + 2, bpm_h + 2, 0, 0, 255);
    }
    led_matrix_fill_rect(bpm_x, bpm_y, bpm_w, bpm_h, 0, 0, 0);
    draw_bpm_number(bpm_x + 1, bpm_y + 1, bpm_get(), 0, 255, 255);

    // PLAY/STOP box (right side)
    int play_x = 41;
    int play_y = 9 + UI_Y_OFFSET;
    int play_w = 17;
    int play_h = 7;

    if (cursor_mode == CURSOR_PLAY) {
        led_matrix_draw_rect(play_x - 1, play_y - 1, play_w + 2, play_h + 2, 0, 0, 255);
    }
    led_matrix_fill_rect(play_x, play_y, play_w, play_h, 0, 0, 0);

    if (is_playing) {
        led_matrix_draw_text(play_x + 1, play_y + 1, "STOP", 255, 0, 0);
    } else {
        led_matrix_draw_text(play_x + 1, play_y + 1, "PLAY", 0, 255, 0);
    }

    // track numbers
    int box_x = 5;
    int box_y_start = 19 + UI_Y_OFFSET;
    int box_w = 7;
    int box_h = 7;
    int spacing = 2;

    const char* labels[4] = {"1", "2", "3", "4"};

    for (int i = 0; i < num_tracks; i++) {
        int y = box_y_start + i * (box_h + spacing);

        bool is_cursor = (cursor_mode == CURSOR_TRACK_LABEL && cursor_track == i);
        bool is_active = (i == active_track);
        bool has_preset = (track_preset[i] != -1);

        if (is_cursor) {
            led_matrix_draw_rect(box_x - 1, y - 1, box_w + 2, box_h + 2, 0, 0, 255);
        }

        if (is_active && has_preset) {
            led_matrix_fill_rect(box_x, y, box_w, box_h, 255, 0, 0);
            led_matrix_draw_text(box_x + 2, y + 1, labels[i], 255, 255, 255);
        } else if (has_preset) {
            led_matrix_fill_rect(box_x, y, box_w, box_h, 255, 255, 255);
            led_matrix_draw_text(box_x + 2, y + 1, labels[i], 0, 0, 0);
        } else {
            led_matrix_fill_rect(box_x, y, box_w, box_h, 0, 0, 0);
            led_matrix_draw_text(box_x + 2, y + 1, labels[i], 255, 255, 255);
        }
    }

    draw_grid(num_tracks);

    // Draw presets at bottom ONLY if in preset selection mode
    if (preset_selection_mode) {
        int preset_y = 56 + UI_Y_OFFSET;
        int preset_x_start = 4;
        int preset_spacing = 14;

        for (int i = 0; i < NUM_PRESETS; i++) {
            int x = preset_x_start + i * preset_spacing;
            bool is_preset_cursor = (i == preset_cursor);

            uint8_t r = (is_preset_cursor) ? 0 : 80;
            uint8_t g = (is_preset_cursor) ? 0 : 80;
            uint8_t b = (is_preset_cursor) ? 255 : 80;

            draw_waveform(x, preset_y, i, r, g, b);
        }
    }
}

// UI initialization
void ui_init(void) {
    buttons_init();
    bpm_init();

    // Install shared ISR for all buttons
    for (int i = 0; i < 5; i++) {
        gpio_set_irq_enabled_with_callback(BUTTON_PINS[i],
                                           GPIO_IRQ_EDGE_FALL |
                                               GPIO_IRQ_EDGE_RISE, // detect press + release
                                           true, &button_isr);
    }

    draw_menu();
}

// UI update (called in main loop)
void ui_update(void) {
    // Process debouncing for all 5 buttons
    for (int b = 0; b < 5; b++) {
        process_button(b);
    }

    // ----- UP BUTTON -----
    if (button_short_press[BUTTON_UP]) {
        button_short_press[BUTTON_UP] = false;

        if (preset_selection_mode) {
            // In preset selection mode, do nothing (use left/right to navigate presets)
        } else if (cursor_mode == CURSOR_BEAT || cursor_mode == CURSOR_TRACK_LABEL) {
            // Move up in tracks
            if (cursor_track > 0) {
                cursor_track--;
            } else {
                // At top track, move to BPM or PLAY
                if (cursor_mode == CURSOR_BEAT) {
                    if (cursor_beat >= NUM_BEATS / 2) {
                        cursor_mode = CURSOR_PLAY;
                    } else {
                        cursor_mode = CURSOR_BPM;
                    }
                } else {
                    cursor_mode = CURSOR_BPM;
                }
            }
        }
    }

    // ----- DOWN BUTTON -----
    if (button_short_press[BUTTON_DOWN]) {
        button_short_press[BUTTON_DOWN] = false;

        if (preset_selection_mode) {
            // In preset selection mode, do nothing
        } else if (cursor_mode == CURSOR_BPM || cursor_mode == CURSOR_PLAY) {
            cursor_mode = CURSOR_TRACK_LABEL;
            cursor_track = 0;
        } else if (cursor_mode == CURSOR_BEAT || cursor_mode == CURSOR_TRACK_LABEL) {
            if (cursor_track < num_tracks - 1) {
                cursor_track++;
            }
        }
    }

    // ----- LEFT BUTTON -----
    if (button_short_press[BUTTON_LEFT]) {
        button_short_press[BUTTON_LEFT] = false;

        if (preset_selection_mode) {
            if (preset_cursor > 0) {
                preset_cursor--;
            }
        } else if (cursor_mode == CURSOR_PLAY) {
            cursor_mode = CURSOR_BPM;
        } else if (cursor_mode == CURSOR_BEAT) {
            if (cursor_beat > 0) {
                cursor_beat--;
            } else {
                cursor_mode = CURSOR_TRACK_LABEL;
            }
        } else if (cursor_mode == CURSOR_TRACK_LABEL) {
            // no-op or wrap
        }
    }

    // ----- RIGHT BUTTON -----
    if (button_short_press[BUTTON_RIGHT]) {
        button_short_press[BUTTON_RIGHT] = false;

        if (preset_selection_mode) {
            if (preset_cursor < NUM_PRESETS - 1) {
                preset_cursor++;
            }
        } else if (cursor_mode == CURSOR_BPM) {
            cursor_mode = CURSOR_PLAY;
        } else if (cursor_mode == CURSOR_TRACK_LABEL) {
            cursor_mode = CURSOR_BEAT;
            cursor_beat = 0;
        } else if (cursor_mode == CURSOR_BEAT) {
            if (cursor_beat < NUM_BEATS - 1) {
                cursor_beat++;
            }
        }
    }

    // ----- SELECT BUTTON SHORT PRESS -----
    if (button_short_press[BUTTON_SELECT]) {
        button_short_press[BUTTON_SELECT] = false;

        if (preset_selection_mode) {
            // Confirm preset and exit preset mode
            track_preset[preset_selection_track] = preset_cursor;
            preset_selection_mode = false;

            int old_active = active_track;
            if (track_preset[active_track] == -1) {
                active_track = preset_selection_track;
            }

            redraw_track_label(preset_selection_track);
            if (old_active != active_track) {
                redraw_track_label(old_active);
            }
        } else if (cursor_mode == CURSOR_BPM) {
            bpm_tap();
        } else if (cursor_mode == CURSOR_PLAY) {
            is_playing = !is_playing;
            if (is_playing) {
                current_beat = 0;
                last_beat_time = time_us_64();
                for (int t = 0; t < 4; t++) {
                    for (int b = 0; b < NUM_BEATS; b++) {
                        redraw_beat_cell(t, b);
                    }
                }
            } else {
                for (int t = 0; t < 4; t++) {
                    for (int b = 0; b < NUM_BEATS; b++) {
                        redraw_beat_cell(t, b);
                    }
                }
            }
            redraw_play_cursor();
        } else if (cursor_mode == CURSOR_TRACK_LABEL) {
            preset_selection_mode = true;
            preset_selection_track = cursor_track;
            preset_cursor = (track_preset[cursor_track] != -1) ? track_preset[cursor_track] : 0;
        } else if (cursor_mode == CURSOR_BEAT) {
            if (track_preset[cursor_track] != -1) {
                beat_state[cursor_track][cursor_beat] = !beat_state[cursor_track][cursor_beat];
                redraw_beat_cell(cursor_track, cursor_beat);
            }
        }
    }

    // ----- LONG PRESS ON ANY BUTTON WHILE ON TRACK AREA -> SET ACTIVE TRACK -----
    if (button_long_press[BUTTON_SELECT] || button_long_press[BUTTON_UP] ||
        button_long_press[BUTTON_DOWN] || button_long_press[BUTTON_LEFT] ||
        button_long_press[BUTTON_RIGHT]) {

        if (!preset_selection_mode &&
            (cursor_mode == CURSOR_TRACK_LABEL || cursor_mode == CURSOR_BEAT)) {
            if (track_preset[cursor_track] != -1) {
                active_track = cursor_track;
            }
        }

        button_long_press[BUTTON_SELECT] = false;
        button_long_press[BUTTON_UP] = false;
        button_long_press[BUTTON_DOWN] = false;
        button_long_press[BUTTON_LEFT] = false;
        button_long_press[BUTTON_RIGHT] = false;
    }

    // Selective redraw - only update what changed

    // Preset mode changes
    if (preset_selection_mode != prev_preset_selection_mode) {
        draw_presets_area();
        prev_preset_selection_mode = preset_selection_mode;
        prev_preset_cursor = preset_cursor;
    } else if (preset_selection_mode && preset_cursor != prev_preset_cursor) {
        draw_presets_area();
        prev_preset_cursor = preset_cursor;
    }

    // Cursor mode changes
    if (cursor_mode != prev_cursor_mode) {
        if (prev_cursor_mode == CURSOR_BPM)
            redraw_bpm_cursor();
        else if (prev_cursor_mode == CURSOR_PLAY)
            redraw_play_cursor();
        else if (prev_cursor_mode == CURSOR_TRACK_LABEL)
            redraw_track_label(prev_cursor_track);
        else if (prev_cursor_mode == CURSOR_BEAT)
            redraw_beat_cell(prev_cursor_track, prev_cursor_beat);

        if (cursor_mode == CURSOR_BPM)
            redraw_bpm_cursor();
        else if (cursor_mode == CURSOR_PLAY)
            redraw_play_cursor();
        else if (cursor_mode == CURSOR_TRACK_LABEL)
            redraw_track_label(cursor_track);
        else if (cursor_mode == CURSOR_BEAT)
            redraw_beat_cell(cursor_track, cursor_beat);

        prev_cursor_mode = cursor_mode;
        prev_cursor_track = cursor_track;
        prev_cursor_beat = cursor_beat;
    }
    // Cursor position changes within same mode
    else if (cursor_mode == CURSOR_TRACK_LABEL && cursor_track != prev_cursor_track) {
        redraw_track_label(prev_cursor_track);
        redraw_track_label(cursor_track);
        prev_cursor_track = cursor_track;
    } else if (cursor_mode == CURSOR_BEAT) {
        if (cursor_track != prev_cursor_track || cursor_beat != prev_cursor_beat) {
            redraw_beat_cell(prev_cursor_track, prev_cursor_beat);
            redraw_beat_cell(cursor_track, cursor_beat);
            prev_cursor_track = cursor_track;
            prev_cursor_beat = cursor_beat;
        }
    }

    // Active track changes
    if (active_track != prev_active_track) {
        redraw_track_label(prev_active_track);
        redraw_track_label(active_track);
        prev_active_track = active_track;
    }

    // BPM changes
    int current_bpm = bpm_get();
    if (current_bpm != prev_bpm) {
        redraw_bpm_cursor();
        prev_bpm = current_bpm;
    }

    // BPM flash effect
    static bool was_flashing = false;
    bool is_flashing = bpm_is_flashing();
    if (was_flashing && !is_flashing) {
        redraw_bpm_cursor();
    }
    was_flashing = is_flashing;
    bpm_update_flash();

    // Update playback position if playing
    if (is_playing) {
        uint64_t now = time_us_64();
        uint64_t beat_interval_us = 60000000 / bpm_get();

        if ((now - last_beat_time) >= beat_interval_us) {
            int prev_beat = current_beat;

            current_beat++;
            if (current_beat >= NUM_BEATS) {
                current_beat = 0;
            }

            last_beat_time = now;

            for (int t = 0; t < 4; t++) {
                redraw_beat_cell(t, prev_beat);
                redraw_beat_cell(t, current_beat);
            }
        }
    }
}

// Get current BPM value
int ui_get_bpm(void) {
    return bpm_get();
}

int ui_get_active_track(void) {
    return active_track;
}

// Get preset for a specific track (-1 if none)
int ui_get_track_preset(int track) {
    if (track < 0 || track >= 4)
        return -1;
    return track_preset[track];
}

// Check if playing
bool ui_is_playing(void) {
    return is_playing;
}

// Get current beat position (0-7)
int ui_get_current_beat(void) {
    return current_beat;
}

// Check if a specific beat is active for a track
bool ui_get_beat_state(int track, int beat) {
    if (track < 0 || track >= 4 || beat < 0 || beat >= NUM_BEATS)
        return false;
    return beat_state[track][beat];
}
