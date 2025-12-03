#ifndef UI_H
#define UI_H

#include <stdbool.h>

// Initialize UI
void ui_init(void);

// Update UI
void ui_update(void);

// Get current BPM value
int ui_get_bpm(void);

#endif // UI_H
