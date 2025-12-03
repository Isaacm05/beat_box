#ifndef UI_H
#define UI_H

#include <stdbool.h>

// Initialize UI
void ui_init(void);

// Update UI
void ui_update(void);

// Get current BPM value
int ui_get_bpm(void);

// Get active track index (0-3)
int ui_get_active_track(void);

// Get preset for a specific track (-1 if none)
int ui_get_track_preset(int track);

// Check if playing
bool ui_is_playing(void);

// Get current beat position (0-7)
int ui_get_current_beat(void);

// Check if a specific beat is active for a track
bool ui_get_beat_state(int track, int beat);

#endif // UI_H
