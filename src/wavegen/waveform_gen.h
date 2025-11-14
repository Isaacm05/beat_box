#ifndef WAVEFORM_GEN_H
#define WAVEFORM_GEN_H

#include <stdint.h>


    {60.0, 1.0, 0.25, 0, 0.0, 8.0, 0, 4.0, 0.5},    // 0:Kick
    {250.0, 0.8, 0.15, 0, 0.0, 0.8, 0.8, 5.0, 0.6}, // 1:Snare
    {8000.0, 0.5, 0.05, 4, 0.0, 0, 1.0, 6.0, 0.3},  // 2:Hi-Hat
    {55.0, 1.0, 1.2, 0, 0.0, 2.0, 0, 2.5, 0.25},    // 3:808
    {440.0, 0.7, 0.5, 2, 0.0, 0, 0, 0, 0.2},        // 4:Tone
    {8000.0, 0.5, 0.25, 4, 0.0, 0, 1.0, 3.0, 0.4}   // 5:Open Hat

typedef struct {
    float frequency;    // Pot 0            20-9kHz
    float amplitude;    // Pot 1            0.0-1.0
    float decay;        // Pot 2            0.0-2.0s
    int waveform_id;    // 0=sine, 1=square, 2=triangle, 3=saw, 4=noise
    float offset_dc;    // Pot 3            0.0-1.0

    float pitch_decay;  // Pot 4            0.0-10.0
    float noise_mix;    // Pot 5            0.0-1.0
    float env_curve;    // Pot 6            0.0-10.0
    float comp_amount;  // Pot 7            0.0-1.0
} WaveParams;

int waveform_generate(float* buffer, int max_samples, WaveParams* p);

#endif