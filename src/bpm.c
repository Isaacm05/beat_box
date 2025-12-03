#include "bpm.h"
#include "pico/stdlib.h"

// Tap tempo settings
#define MAX_TAP_INTERVAL_US 2000000  // 2 seconds max between taps
#define MIN_TAPS 2                   // Need at least 2 taps
#define MAX_TAPS 4                   // Average last 4 taps
#define BPM_FLASH_DURATION_US 100000 // 100ms flash

// BPM state
static int current_bpm = 120;

// Tap tempo state
static uint64_t tap_times[MAX_TAPS] = {0};
static int tap_count = 0;

// Flash effect state
static bool flash_active = false;
static uint64_t flash_start_time = 0;

// init bpm
void bpm_init(void) {
    current_bpm = 120;
    tap_count = 0;
    flash_active = false;
    for (int i = 0; i < MAX_TAPS; i++) {
        tap_times[i] = 0;
    }
}


int bpm_get(void) {
    return current_bpm;
}

// set BPM
void bpm_set(int bpm) {
    if (bpm < MIN_BPM)
        bpm = MIN_BPM;
    if (bpm > MAX_BPM)
        bpm = MAX_BPM;
    current_bpm = bpm;
}

// tempo tapper
bool bpm_tap(void) {
    uint64_t now = time_us_64();

    // Trigger flash effect
    flash_active = true;
    flash_start_time = now;

    // reset if time elapsed
    if (tap_count > 0 && (now - tap_times[tap_count - 1]) > MAX_TAP_INTERVAL_US) {
        tap_count = 0;
    }

    // add current tap 
    if (tap_count < MAX_TAPS) {
        tap_times[tap_count] = now;
        tap_count++;
    } else {
        // Shift array left and add new tap
        for (int i = 0; i < MAX_TAPS - 1; i++) {
            tap_times[i] = tap_times[i + 1];
        }
        tap_times[MAX_TAPS - 1] = now;
    }

    // Calculate BPM if we have enough taps
    if (tap_count >= MIN_TAPS) {
        uint64_t total_interval = tap_times[tap_count - 1] - tap_times[0];
        int num_intervals = tap_count - 1;
        uint64_t avg_interval = total_interval / num_intervals;

        // Convert to BPM (60 seconds / interval in seconds)
        int calculated_bpm = (int) (60000000.0 / avg_interval);

        // Clamp to valid range
        if (calculated_bpm < MIN_BPM)
            calculated_bpm = MIN_BPM;
        if (calculated_bpm > MAX_BPM)
            calculated_bpm = MAX_BPM;

        current_bpm = calculated_bpm;
        return true; // BPM was updated
    }

    return false; // Not enough taps yet
}

// check flash
bool bpm_is_flashing(void) {
    if (!flash_active)
        return false;

    uint64_t now = time_us_64();
    return (now - flash_start_time < BPM_FLASH_DURATION_US);
}

// Update flash timer
void bpm_update_flash(void) {
    if (flash_active) {
        uint64_t now = time_us_64();
        if (now - flash_start_time >= BPM_FLASH_DURATION_US) {
            flash_active = false;
        }
    }
}
