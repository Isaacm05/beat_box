#include "presets.h"

// frequency, amplitude, decay, waveform_id, offset_dc, pitch_decay, noise_mix, env_curve,
//  comp_amount} waveform_id: 0=sine, 1=square, 2=triangle, 3=saw, 4=noise
WaveParams drum_presets[] = {
    // Original drum presets
    {60.0, 1.0, 0.25, 0, 0.0, 8.0, 0, 4.0, 0.5},    // 0: Kick (Sine)
    {250.0, 0.8, 0.15, 0, 0.0, 0.8, 0.8, 5.0, 0.6}, // 1: Snare (Sine)
    {8000.0, 0.5, 0.05, 4, 0.0, 0, 1.0, 6.0, 0.3},  // 2: Hi-Hat (Noise)
    {55.0, 1.0, 1.2, 0, 0.0, 2.0, 0, 2.5, 0.25},    // 3: 808 Bass (Sine)
    {440.0, 0.7, 0.5, 2, 0.0, 0, 0, 0, 0.2},        // 4: Tone (Triangle)
    {8000.0, 0.5, 0.25, 4, 0.0, 0, 1.0, 3.0, 0.4},  // 5: Open Hat (Noise)

    // basic shapes
    {440.0, 0.7, 0.5, 0, 0.0, 0, 0, 0, 0},   // 6: Pure Sine
    {440.0, 0.7, 0.5, 2, 0.0, 0, 0, 0, 0},   // 7: Pure Triangle
    {440.0, 0.7, 0.5, 1, 0.0, 0, 0, 0, 0},   // 8: Pure Square (Rectangle)
    {440.0, 0.7, 0.5, 4, 0.0, 0, 1.0, 0, 0}, // 9: Pure Noise
};

const int num_presets = sizeof(drum_presets) / sizeof(WaveParams);
