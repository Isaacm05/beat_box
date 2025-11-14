#include "waveform_gen.h"

static WaveParams drum_presets[] = {

    {60.0, 1.0, 0.25, 0, 0.0, 8.0, 0, 4.0, 0.5},    // 0:Kick
    {250.0, 0.8, 0.15, 0, 0.0, 0.8, 0.8, 5.0, 0.6}, // 1:Snare
    {8000.0, 0.5, 0.05, 4, 0.0, 0, 1.0, 6.0, 0.3},  // 2:Hi-Hat
    {55.0, 1.0, 1.2, 0, 0.0, 2.0, 0, 2.5, 0.25},    // 3:808
    {440.0, 0.7, 0.5, 2, 0.0, 0, 0, 0, 0.2},        // 4:Tone
    {8000.0, 0.5, 0.25, 4, 0.0, 0, 1.0, 3.0, 0.4}   // 5:Open Hat
};

static const int num_presets = sizeof(drum_presets) / sizeof(WaveParams);
