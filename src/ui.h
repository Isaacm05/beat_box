#ifndef UI_H
#define UI_H

#include <stdbool.h>

// Initialize UI system (buttons, display initial state)
void ui_init(void);

// Update UI (process buttons, handle state changes, selective redraw)
void ui_update(void);

#endif // UI_H
