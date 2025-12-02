#include "led_matrix.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <time.h>
 
#define TARGET_FPS 60
#define FRAME_TIME_US (1000000 / TARGET_FPS)
#define COLOR_CHANGE_INTERVAL_US 1000000 // 1 second
#define DEBOUNCE_US      15000     // 15 ms debounce
#define LONG_PRESS_US   400000     // 400 ms for long-press


// color control vars
const uint8_t colors[][3] = {
        {255,   0,   0},  // Red
        {  0, 255,   0},  // Green
        {  0,   0, 255},  // Blue
        {255, 255,   0},  // Yellow
        {255,   0, 255},  // Magenta
        {  0, 255, 255},  // Cyan
        {255, 255, 255},  // White
        {  0,   0,   0},  // Black
};
const int num_colors = 8;


// button control vars
const uint BUTTON_PINS[4] = {
    PIN_LEFT,
    PIN_RIGHT,
    PIN_UP,
    PIN_DOWN
};

enum Button {
    BUTTON_LEFT = 0,
    BUTTON_RIGHT = 1,
    BUTTON_UP = 2,
    BUTTON_DOWN = 3
};

volatile bool button_short_press[4] = {0};
volatile bool button_long_press[4]  = {0};

volatile bool button_raw_event[4] = {0};
bool last_raw_state[4]            = {0};
bool stable_state[4]              = {0};
uint64_t last_change_time[4]      = {0};
uint64_t press_start_time[4]      = {0};


// UI menu vars
int menu_index = 0;  // scrolls through tracks   
int num_tracks = 1;     // min 1 track, max 4
int selected_track = 0; 
bool track_selected_mode = false;
int selected_track_final = 0;  



// button initialization 
void buttons_init(void) {
    for (int i = 0; i < 4; i++) {
        gpio_init(BUTTON_PINS[i]);
        gpio_set_dir(BUTTON_PINS[i], GPIO_IN);
        gpio_pull_up(BUTTON_PINS[i]);  // active LOW
    }
}

// button interrupt service routine
void button_isr(uint gpio, uint32_t events) {
    for (int i = 0; i < 4; i++) {
        if (gpio == BUTTON_PINS[i]) {
            button_raw_event[i] = true;
            last_change_time[i] = time_us_64();  
        }
    }
}

// process button state with debounce
void process_button(int b) {
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
            // RELEASED — measure duration
            uint64_t press_len = now - press_start_time[b];

            if (press_len >= LONG_PRESS_US) {
                button_long_press[b] = true;
            } else {
                button_short_press[b] = true;
            }
        }
    }
}

// draw grid for tracks
void draw_grid(int num_tracks) {
    int grid_x = 15;      // left side of grid
    int grid_y = 21;      // below title
    int cell_w = 3;
    int cell_h = 3;
    int spacing = 1;

    // grid cells
    for (int row = 0; row < num_tracks; row++) {
        for (int col = 0; col < 12; col++) {

            int x = grid_x + col * (cell_w + spacing);
            int y = grid_y + row * (cell_h + 7*spacing);

            // white outline
            led_matrix_draw_rect(x, y, cell_w, cell_h,
                                 200, 200, 200);

            // black fill
            led_matrix_fill_rect(x+1, y+1, cell_w-2, cell_h-2,
                                 0, 0, 0);
        }
    }
}

// draw UI menu 
void draw_menu(int selected) {
    led_matrix_clear();

    // title 
    const char *title = "BEAT BOX";
    int title_len = 8;
    int char_w = 4;            
    int title_px = title_len * char_w;
    int title_x = (64 - title_px) / 2;

    led_matrix_draw_text(title_x, 2, title, 255, 255, 0);

    // bpm box
    int bpm_x = 4;
    int bpm_y = 9;    
    int bpm_w = 15;
    int bpm_h = 7;     

    led_matrix_fill_rect(bpm_x, bpm_y, bpm_w, bpm_h,
                          0, 255, 255);  // blue fill
    led_matrix_draw_text(bpm_x + 2, bpm_y + 1, "BPM", 0, 0, 0);


    // track numbers
    int box_x = 4;
    int box_y_start = 19;
    int box_w = 7;
    int box_h = 7;
    int spacing = 3;

    const char *labels[4] = { "1", "2", "3", "4" };

    for (int i = 0; i < num_tracks; i++) {
        int y = box_y_start + i * (box_h + spacing);

        led_matrix_fill_rect(box_x, y, box_w, box_h, 255, 255, 255);
        led_matrix_draw_text(box_x + 2, y + 1, labels[i], 0, 0, 0);

        if (selected == i) 
            led_matrix_draw_rect(box_x - 1, y - 1, box_w + 2, box_h + 2, 0, 255, 0);
    }

    draw_grid(num_tracks);

    // add red rectangle if track selected
    if (track_selected_mode) {

        int box_x = 4;
        int box_y_start = 19;
        int box_w = 7;
        int box_h = 7;
        int spacing = 3;

        int track_y = box_y_start + selected_track_final * (box_h + spacing);

        int grid_x = 15;
        int grid_y = 21;
        int cell_w = 3;
        int cell_h = 3;
        int cell_spacing = 1;

        int grid_row_y = grid_y + selected_track_final * (cell_h + 7 * cell_spacing);

        int grid_width = 12 * (cell_w + cell_spacing) - cell_spacing;

        // Rectangle boundaries
        int select_x = box_x - 3;                    // start a little before track box
        int select_y = track_y - 3;                  // top boundary
        int select_w = (grid_x + grid_width) - select_x + 5;
        int select_h = (grid_row_y + cell_h) - select_y + 5;

        led_matrix_draw_rect(select_x, select_y, select_w, select_h,
                             255, 0, 0);   // RED rectangle
    }

}



// main() — combines LED color cycling + 4-button debounce
int main() {
    stdio_init_all();
    sleep_ms(2000);

    led_matrix_init();
    buttons_init();

    // Install shared ISR for all buttons
    for (int i = 0; i < 4; i++) {
        gpio_set_irq_enabled_with_callback(
            BUTTON_PINS[i],
            GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,   // detect press + release
            true,
            &button_isr
        );
    }

    draw_menu(selected_track);

    
    while (1) {
    led_matrix_refresh();

    // Process debouncing for all 4 buttons
    for (int b = 0; b < 4; b++) {
        process_button(b);
    }

    // UP - moves select up
    if (!track_selected_mode && button_short_press[BUTTON_UP]) {
        button_short_press[BUTTON_UP] = false;

        if (num_tracks > 0) {
            selected_track--;
            if (selected_track < 0) {
                selected_track = num_tracks - 1;   // wrap to last
            }
            draw_menu(selected_track);
        }
    }

    // UP - LONG press to select track
    if (button_long_press[BUTTON_UP]) {
        button_long_press[BUTTON_UP] = false;

        // Enter selection mode
        track_selected_mode = true;
        selected_track_final = selected_track;

        draw_menu(selected_track);
        printf("Track %d selected (LONG UP)\n", selected_track_final + 1);
    }


    // DOWN - moves select down
    if (!track_selected_mode && button_short_press[BUTTON_DOWN]) {
        button_short_press[BUTTON_DOWN] = false;

        if (num_tracks > 0) {
            selected_track = (selected_track + 1) % num_tracks;
            draw_menu(selected_track);
        }
    }

    // DOWN - LONG press to deselect track
    if (button_long_press[BUTTON_DOWN]) {
        button_long_press[BUTTON_DOWN] = false;

        if (track_selected_mode) {
            track_selected_mode = false;

            draw_menu(selected_track);

            printf("Track %d deselected (LONG DOWN)\n", selected_track_final + 1);
        }
    }


    // RIGHT - add track 
    if (button_short_press[BUTTON_RIGHT]) {
        button_short_press[BUTTON_RIGHT] = false;

        if (num_tracks < 4) {
            num_tracks++;
            selected_track = num_tracks - 1;
        }

        draw_menu(selected_track);
        printf("Tracks: %d (RIGHT)\n", num_tracks);
    }

    // LEFT - remove track 
    if (button_short_press[BUTTON_LEFT]) {
        button_short_press[BUTTON_LEFT] = false;

        if (num_tracks > 1) {
            num_tracks--;

            if (selected_track >= num_tracks) {
                selected_track = num_tracks - 1;
            }
        }

        draw_menu(selected_track);
        printf("Tracks: %d (LEFT)\n", num_tracks);
    }
}

}
