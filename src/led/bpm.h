#ifndef BPM_H
#define BPM_H

#include <stdbool.h>
#include <stdint.h>

// BPM limits
#define MIN_BPM 60
#define MAX_BPM 240

// Initialize BPM
void bpm_init(void);

// Get current BPM value
int bpm_get(void);

// Set BPM value
void bpm_set(int bpm);

// call this on each bpm tap
bool bpm_tap(void);

// flash bpm
bool bpm_is_flashing(void);

// Update flash timer
void bpm_update_flash(void);

#endif // BPM_H
