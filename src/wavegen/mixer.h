#ifndef MIXER_H
#define MIXER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_MIXER_TRACKS 4

// Initialize the mixer
void mixer_init(void);

// Add a track to the mix buffer
// track_id: 0-3 for track identification
// buffer: audio samples to add
// len: number of samples
void mixer_add_track(int track_id, const uint16_t* buffer, int len);

// Get the current mixed output buffer
const uint16_t* mixer_get_output(int* len);

// Clear the mix buffer for next cycle
void mixer_clear(void);

// Check if mixer has any active tracks
bool mixer_has_active_tracks(void);

#endif
