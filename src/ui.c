#include "ui.h"
#include "led_matrix.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <time.h>

#define TARGET_FPS 60
#define FRAME_TIME_US (1000000 / TARGET_FPS)
#define COLOR_CHANGE_INTERVAL_US 1000000 // 1 second
#define DEBOUNCE_US 15000                // 15 ms debounce
#define LONG_PRESS_US 800000             // 800 ms for long-press

// color control vars
static const uint8_t colors[][3] = {
    {255, 0, 0},     // Red
    {0, 255, 0},     // Green
    {0, 0, 255},     // Blue
    {255, 255, 0},   // Yellow
    {255, 0, 255},   // Magenta
    {0, 255, 255},   // Cyan
    {255, 255, 255}, // White
    {0, 0, 0},       // Black
};
static const int num_colors = 8;

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

// Track presets (-1 = no preset selected, 0-3 = preset index)
static int track_preset[4] = {-1, -1, -1, -1};

// Preset selection mode
static bool preset_selection_mode = false;
static bool prev_preset_selection_mode = false;
static int preset_selection_track = 0;
static int preset_cursor = 0;

// Beat state for each track (true = filled/active)
static bool beat_state[4][NUM_BEATS] = {0};

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
            // RELEASED  measure duration
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
    int grid_y = 20;
    int cell_w = 4;
    int cell_h = 4;
    int spacing = 2;
    int row_spacing = 5;

    int x = grid_x + beat * (cell_w + spacing);
    int y = grid_y + track * (cell_h + row_spacing);

    bool is_cursor = (cursor_mode == CURSOR_BEAT && cursor_track == track && cursor_beat == beat);
    bool is_filled = beat_state[track][beat];

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

// Redraw track label
static void redraw_track_label(int track) {
    int box_x = 5;
    int box_y_start = 19;
    int box_w = 7;
    int box_h = 7;
    int spacing = 2;

    const char* labels[4] = {"1", "2", "3", "4"};
    int y = box_y_start + track * (box_h + spacing);

    bool is_cursor = (cursor_mode == CURSOR_TRACK_LABEL && cursor_track == track);
    bool is_active = (track == active_track);

    if (is_cursor) {
        led_matrix_draw_rect(box_x - 1, y - 1, box_w + 2, box_h + 2, 0, 0, 255);
    } else {
        // Clear cursor outline
        led_matrix_draw_rect(box_x - 1, y - 1, box_w + 2, box_h + 2, 0, 0, 0);
    }

    if (is_active) {
        led_matrix_fill_rect(box_x, y, box_w, box_h, 255, 0, 0);
        led_matrix_draw_text(box_x + 2, y + 1, labels[track], 255, 255, 255);
    } else {
        led_matrix_fill_rect(box_x, y, box_w, box_h, 255, 255, 255);
        led_matrix_draw_text(box_x + 2, y + 1, labels[track], 0, 0, 0);
    }
}

// Redraw BPM box cursor
static void redraw_bpm_cursor() {
    int bpm_x = 4;
    int bpm_y = 9;
    int bpm_w = 15;
    int bpm_h = 7;

    if (cursor_mode == CURSOR_BPM) {
        led_matrix_draw_rect(bpm_x - 1, bpm_y - 1, bpm_w + 2, bpm_h + 2, 0, 0, 255);
    } else {
        led_matrix_draw_rect(bpm_x - 1, bpm_y - 1, bpm_w + 2, bpm_h + 2, 0, 0, 0);
    }
}

// Redraw PLAY box cursor
static void redraw_play_cursor() {
    int play_x = 41;
    int play_y = 9;
    int play_w = 17;
    int play_h = 7;

    if (cursor_mode == CURSOR_PLAY) {
        led_matrix_draw_rect(play_x - 1, play_y - 1, play_w + 2, play_h + 2, 0, 0, 255);
    } else {
        led_matrix_draw_rect(play_x - 1, play_y - 1, play_w + 2, play_h + 2, 0, 0, 0);
    }
}

// Draw or clear presets area
static void draw_presets_area() {
    int preset_y = 56;
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

        if (i == 0) {
            // Sine wave
            led_matrix_set_pixel(x, preset_y + 2, r, g, b);
            led_matrix_set_pixel(x + 1, preset_y + 1, r, g, b);
            led_matrix_set_pixel(x + 2, preset_y, r, g, b);
            led_matrix_set_pixel(x + 3, preset_y + 1, r, g, b);
            led_matrix_set_pixel(x + 4, preset_y + 2, r, g, b);
            led_matrix_set_pixel(x + 5, preset_y + 3, r, g, b);
            led_matrix_set_pixel(x + 6, preset_y + 4, r, g, b);
            led_matrix_set_pixel(x + 7, preset_y + 3, r, g, b);
            led_matrix_set_pixel(x + 8, preset_y + 2, r, g, b);
        } else if (i == 1) {
            // Triangle wave
            led_matrix_set_pixel(x, preset_y + 4, r, g, b);
            led_matrix_set_pixel(x + 1, preset_y + 3, r, g, b);
            led_matrix_set_pixel(x + 2, preset_y + 2, r, g, b);
            led_matrix_set_pixel(x + 3, preset_y + 1, r, g, b);
            led_matrix_set_pixel(x + 4, preset_y, r, g, b);
            led_matrix_set_pixel(x + 5, preset_y + 1, r, g, b);
            led_matrix_set_pixel(x + 6, preset_y + 2, r, g, b);
            led_matrix_set_pixel(x + 7, preset_y + 3, r, g, b);
            led_matrix_set_pixel(x + 8, preset_y + 4, r, g, b);
        } else if (i == 2) {
            // Square wave
            led_matrix_set_pixel(x, preset_y, r, g, b);
            led_matrix_set_pixel(x + 1, preset_y, r, g, b);
            led_matrix_set_pixel(x + 2, preset_y, r, g, b);
            led_matrix_set_pixel(x + 3, preset_y, r, g, b);
            led_matrix_set_pixel(x + 4, preset_y + 4, r, g, b);
            led_matrix_set_pixel(x + 5, preset_y + 4, r, g, b);
            led_matrix_set_pixel(x + 6, preset_y + 4, r, g, b);
            led_matrix_set_pixel(x + 7, preset_y + 4, r, g, b);
            led_matrix_set_pixel(x + 8, preset_y, r, g, b);
        } else {
            // Noise
            led_matrix_set_pixel(x, preset_y + 1, r, g, b);
            led_matrix_set_pixel(x + 1, preset_y + 3, r, g, b);
            led_matrix_set_pixel(x + 2, preset_y, r, g, b);
            led_matrix_set_pixel(x + 3, preset_y + 4, r, g, b);
            led_matrix_set_pixel(x + 4, preset_y + 2, r, g, b);
            led_matrix_set_pixel(x + 5, preset_y, r, g, b);
            led_matrix_set_pixel(x + 6, preset_y + 3, r, g, b);
            led_matrix_set_pixel(x + 7, preset_y + 1, r, g, b);
            led_matrix_set_pixel(x + 8, preset_y + 4, r, g, b);
        }
    }
}

// draw grid for tracks (now 9 beats)
static void draw_grid(int num_tracks) {
    int grid_x = 14; // left side of grid
    int grid_y = 20; // below title
    int cell_w = 4;  // square size
    int cell_h = 4;  // square size
    int spacing = 2;
    int row_spacing = 5; // vertical spacing between rows

    // grid cells
    for (int row = 0; row < num_tracks; row++) {
        for (int col = 0; col < NUM_BEATS; col++) {

            int x = grid_x + col * (cell_w + spacing);
            int y = grid_y + row * (cell_h + row_spacing);

            // Check if this is the cursor position
            bool is_cursor =
                (cursor_mode == CURSOR_BEAT && cursor_track == row && cursor_beat == col);

            // Check if this beat is filled
            bool is_filled = beat_state[row][col];

            // Blue cursor outline or white outline
            if (is_cursor) {
                led_matrix_draw_rect(x, y, cell_w, cell_h, 0, 0, 255);
            } else {
                led_matrix_draw_rect(x, y, cell_w, cell_h, 200, 200, 200);
            }

            // Fill: yellow if filled, black if empty
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

    led_matrix_draw_text(title_x, 2, title, 255, 255, 0);

    // BPM box (left side)
    int bpm_x = 4;
    int bpm_y = 9;
    int bpm_w = 15;
    int bpm_h = 7;

    if (cursor_mode == CURSOR_BPM) {
        led_matrix_draw_rect(bpm_x - 1, bpm_y - 1, bpm_w + 2, bpm_h + 2, 0, 0, 255);
    }
    led_matrix_fill_rect(bpm_x, bpm_y, bpm_w, bpm_h, 0, 255, 255);
    led_matrix_draw_text(bpm_x + 2, bpm_y + 1, "BPM", 0, 0, 0);

    // PLAY box (right side)
    int play_x = 41;
    int play_y = 9;
    int play_w = 17;
    int play_h = 7;

    if (cursor_mode == CURSOR_PLAY) {
        led_matrix_draw_rect(play_x - 1, play_y - 1, play_w + 2, play_h + 2, 0, 0, 255);
    }
    led_matrix_fill_rect(play_x, play_y, play_w, play_h, 0, 255, 0);
    led_matrix_draw_text(play_x + 1, play_y + 1, "PLAY", 0, 0, 0);

    // track numbers
    int box_x = 5;
    int box_y_start = 19;
    int box_w = 7;
    int box_h = 7;
    int spacing = 2;

    const char* labels[4] = {"1", "2", "3", "4"};

    for (int i = 0; i < num_tracks; i++) {
        int y = box_y_start + i * (box_h + spacing);

        // Check if cursor is on this track label
        bool is_cursor = (cursor_mode == CURSOR_TRACK_LABEL && cursor_track == i);

        // Check if this is the active track (show red fill)
        bool is_active = (i == active_track);

        if (is_cursor) {
            led_matrix_draw_rect(box_x - 1, y - 1, box_w + 2, box_h + 2, 0, 0, 255);
        }

        // Fill with red if active, white if not
        if (is_active) {
            led_matrix_fill_rect(box_x, y, box_w, box_h, 255, 0, 0);
            led_matrix_draw_text(box_x + 2, y + 1, labels[i], 255, 255, 255);
        } else {
            led_matrix_fill_rect(box_x, y, box_w, box_h, 255, 255, 255);
            led_matrix_draw_text(box_x + 2, y + 1, labels[i], 0, 0, 0);
        }
    }

    draw_grid(num_tracks);

    // Draw presets at bottom ONLY if in preset selection mode
    if (preset_selection_mode) {
        int preset_y = 56;
        int preset_x_start = 4;
        int preset_spacing = 14;

        for (int i = 0; i < NUM_PRESETS; i++) {
            int x = preset_x_start + i * preset_spacing;

            // Highlight the cursor selection with blue outline
            bool is_preset_cursor = (i == preset_cursor);

            uint8_t r = (is_preset_cursor) ? 0 : 80;
            uint8_t g = (is_preset_cursor) ? 0 : 80;
            uint8_t b = (is_preset_cursor) ? 255 : 80;

            // Draw different wave shapes
            if (i == 0) {
                // Sine wave (smooth curve)
                led_matrix_set_pixel(x, preset_y + 2, r, g, b);
                led_matrix_set_pixel(x + 1, preset_y + 1, r, g, b);
                led_matrix_set_pixel(x + 2, preset_y, r, g, b);
                led_matrix_set_pixel(x + 3, preset_y + 1, r, g, b);
                led_matrix_set_pixel(x + 4, preset_y + 2, r, g, b);
                led_matrix_set_pixel(x + 5, preset_y + 3, r, g, b);
                led_matrix_set_pixel(x + 6, preset_y + 4, r, g, b);
                led_matrix_set_pixel(x + 7, preset_y + 3, r, g, b);
                led_matrix_set_pixel(x + 8, preset_y + 2, r, g, b);
            } else if (i == 1) {
                // Triangle wave
                led_matrix_set_pixel(x, preset_y + 4, r, g, b);
                led_matrix_set_pixel(x + 1, preset_y + 3, r, g, b);
                led_matrix_set_pixel(x + 2, preset_y + 2, r, g, b);
                led_matrix_set_pixel(x + 3, preset_y + 1, r, g, b);
                led_matrix_set_pixel(x + 4, preset_y, r, g, b);
                led_matrix_set_pixel(x + 5, preset_y + 1, r, g, b);
                led_matrix_set_pixel(x + 6, preset_y + 2, r, g, b);
                led_matrix_set_pixel(x + 7, preset_y + 3, r, g, b);
                led_matrix_set_pixel(x + 8, preset_y + 4, r, g, b);
            } else if (i == 2) {
                // Square wave
                led_matrix_set_pixel(x, preset_y, r, g, b);
                led_matrix_set_pixel(x + 1, preset_y, r, g, b);
                led_matrix_set_pixel(x + 2, preset_y, r, g, b);
                led_matrix_set_pixel(x + 3, preset_y, r, g, b);
                led_matrix_set_pixel(x + 4, preset_y + 4, r, g, b);
                led_matrix_set_pixel(x + 5, preset_y + 4, r, g, b);
                led_matrix_set_pixel(x + 6, preset_y + 4, r, g, b);
                led_matrix_set_pixel(x + 7, preset_y + 4, r, g, b);
                led_matrix_set_pixel(x + 8, preset_y, r, g, b);
            } else {
                // Noise (random dots)
                led_matrix_set_pixel(x, preset_y + 1, r, g, b);
                led_matrix_set_pixel(x + 1, preset_y + 3, r, g, b);
                led_matrix_set_pixel(x + 2, preset_y, r, g, b);
                led_matrix_set_pixel(x + 3, preset_y + 4, r, g, b);
                led_matrix_set_pixel(x + 4, preset_y + 2, r, g, b);
                led_matrix_set_pixel(x + 5, preset_y, r, g, b);
                led_matrix_set_pixel(x + 6, preset_y + 3, r, g, b);
                led_matrix_set_pixel(x + 7, preset_y + 1, r, g, b);
                led_matrix_set_pixel(x + 8, preset_y + 4, r, g, b);
            }
        }
    }
}

// UI initialization
void ui_init(void) {
    buttons_init();

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

    // Debug: show SELECT button state
    static int select_debug_counter = 0;
    select_debug_counter++;

    // Show raw button state every 60 frames (once per second at 60fps)
    if (preset_selection_mode && select_debug_counter % 60 == 0) {
        bool raw = (gpio_get(BUTTON_PINS[BUTTON_SELECT]) == 0);
        printf("SELECT raw=%d, stable=%d, short=%d, long=%d, raw_event=%d\n", raw,
               stable_state[BUTTON_SELECT], button_short_press[BUTTON_SELECT],
               button_long_press[BUTTON_SELECT], button_raw_event[BUTTON_SELECT]);
    }

    if (button_short_press[BUTTON_SELECT] || button_long_press[BUTTON_SELECT]) {
        printf("SELECT flags: short=%d, long=%d, preset_mode=%d\n",
               button_short_press[BUTTON_SELECT], button_long_press[BUTTON_SELECT],
               preset_selection_mode);
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
                    // If on right half, go to PLAY
                    if (cursor_beat >= NUM_BEATS / 2) {
                        cursor_mode = CURSOR_PLAY;
                    } else {
                        cursor_mode = CURSOR_BPM;
                    }
                } else {
                    // From track label, go to BPM
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
            // From BPM or PLAY, go to track 0
            cursor_mode = CURSOR_TRACK_LABEL;
            cursor_track = 0;
        } else if (cursor_mode == CURSOR_BEAT || cursor_mode == CURSOR_TRACK_LABEL) {
            // Move down in tracks
            if (cursor_track < num_tracks - 1) {
                cursor_track++;
            }
        }
    }

    // ----- LEFT BUTTON -----
    if (button_short_press[BUTTON_LEFT]) {
        button_short_press[BUTTON_LEFT] = false;

        if (preset_selection_mode) {
            // Navigate presets left
            if (preset_cursor > 0) {
                preset_cursor--;
            }
        } else if (cursor_mode == CURSOR_PLAY) {
            cursor_mode = CURSOR_BPM;
        } else if (cursor_mode == CURSOR_BEAT) {
            // Move left in beats
            if (cursor_beat > 0) {
                cursor_beat--;
            } else {
                // At leftmost beat, go to track label
                cursor_mode = CURSOR_TRACK_LABEL;
            }
        } else if (cursor_mode == CURSOR_TRACK_LABEL) {
            // Already at leftmost, do nothing or wrap
        }
    }

    // ----- RIGHT BUTTON -----
    if (button_short_press[BUTTON_RIGHT]) {
        button_short_press[BUTTON_RIGHT] = false;

        if (preset_selection_mode) {
            // Navigate presets right
            if (preset_cursor < NUM_PRESETS - 1) {
                preset_cursor++;
            }
        } else if (cursor_mode == CURSOR_BPM) {
            cursor_mode = CURSOR_PLAY;
        } else if (cursor_mode == CURSOR_TRACK_LABEL) {
            // Can always enter beat grid
            cursor_mode = CURSOR_BEAT;
            cursor_beat = 0;
        } else if (cursor_mode == CURSOR_BEAT) {
            // Move right in beats
            if (cursor_beat < NUM_BEATS - 1) {
                cursor_beat++;
            }
        }
    }

    // ----- SELECT BUTTON SHORT PRESS - Set active track OR toggle beat -----
    if (button_short_press[BUTTON_SELECT]) {
        button_short_press[BUTTON_SELECT] = false;
        printf("SELECT short press detected, preset_mode=%d, cursor_mode=%d\n",
               preset_selection_mode, cursor_mode);

        if (cursor_mode == CURSOR_TRACK_LABEL) {
            // Short press on track label sets active track
            active_track = cursor_track;
            printf("Track %d set as active\n", cursor_track + 1);
        } else if (cursor_mode == CURSOR_BEAT) {
            // Toggle beat - but only if track has a preset
            if (track_preset[cursor_track] != -1) {
                beat_state[cursor_track][cursor_beat] = !beat_state[cursor_track][cursor_beat];
                printf("Beat [%d][%d] = %s (preset=%d)\n", cursor_track, cursor_beat,
                       beat_state[cursor_track][cursor_beat] ? "ON" : "OFF",
                       track_preset[cursor_track]);
                // Beat toggle will be redrawn by selective redraw
                redraw_beat_cell(cursor_track, cursor_beat);
            } else {
                printf("Cannot toggle beat - no preset selected for track %d\n", cursor_track + 1);
            }
        }
        // BPM and PLAY short press disabled
    }

    // ----- ANY BUTTON LONG PRESS - Exit preset mode OR set active track -----
    if (button_long_press[BUTTON_SELECT] || button_long_press[BUTTON_UP] ||
        button_long_press[BUTTON_DOWN] || button_long_press[BUTTON_LEFT] ||
        button_long_press[BUTTON_RIGHT]) {

        printf("Long press detected, preset_mode=%d, cursor_track=%d\n", preset_selection_mode,
               cursor_track);

        if (preset_selection_mode) {
            // Long press in preset mode confirms and exits
            track_preset[preset_selection_track] = preset_cursor;
            preset_selection_mode = false;
            printf("Track %d preset set to %d, exiting preset mode\n", preset_selection_track + 1,
                   preset_cursor);
        } else if (cursor_mode == CURSOR_TRACK_LABEL || cursor_mode == CURSOR_BEAT) {
            // Long press on track row sets active track (don't enter preset mode)
            active_track = cursor_track;
            printf("Track %d set as active (long press)\n", cursor_track + 1);
        }

        // Clear all long press flags
        button_long_press[BUTTON_SELECT] = false;
        button_long_press[BUTTON_UP] = false;
        button_long_press[BUTTON_DOWN] = false;
        button_long_press[BUTTON_LEFT] = false;
        button_long_press[BUTTON_RIGHT] = false;
    }

    // Selective redraw - only update what changed

    // Check preset mode changes (show/hide presets area)
    if (preset_selection_mode != prev_preset_selection_mode) {
        draw_presets_area();
        prev_preset_selection_mode = preset_selection_mode;
        prev_preset_cursor = preset_cursor;
    }
    // Check preset cursor changes (only if in preset mode)
    else if (preset_selection_mode && preset_cursor != prev_preset_cursor) {
        draw_presets_area();
        prev_preset_cursor = preset_cursor;
    }

    // Check cursor mode changes
    if (cursor_mode != prev_cursor_mode) {
        // Clear old cursor
        if (prev_cursor_mode == CURSOR_BPM)
            redraw_bpm_cursor();
        else if (prev_cursor_mode == CURSOR_PLAY)
            redraw_play_cursor();
        else if (prev_cursor_mode == CURSOR_TRACK_LABEL)
            redraw_track_label(prev_cursor_track);
        else if (prev_cursor_mode == CURSOR_BEAT)
            redraw_beat_cell(prev_cursor_track, prev_cursor_beat);

        // Draw new cursor
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
    // Check cursor position changes within same mode
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

    // Check active track changes
    if (active_track != prev_active_track) {
        redraw_track_label(prev_active_track);
        redraw_track_label(active_track);
        prev_active_track = active_track;
    }
}
