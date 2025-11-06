#pragma once
#include "waveform_gen.c"

static WaveParams drum_presets[] = {
    {120.0f, 1.0f, 0.3f, 0, 0.0f},   // Kick
    {200.0f, 0.6f, 0.2f, 4, 0.0f},   // Snare
    {8000.0f, 0.5f, 0.05f, 4, 0.0f}, // Hi-hat
    {60.0f, 0.9f, 1.0f, 0, 0.0f}     // 808
};

static const int num_presets = sizeof(drum_presets) / sizeof(WaveParams);
