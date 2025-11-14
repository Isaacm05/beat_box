#ifndef WAVEFORM_GEN_H
#define WAVEFORM_GEN_H

#include <stdint.h>

typedef struct {
    float frequency;    // Pot 0
    float amplitude;    // Pot 1
    float decay;        // Pot 2
    int waveform_id;    // 0=sine, 1=square, 2=triangle, 3=saw, 4=noise
    float offset_dc;    // Pot 3

    float pitch_decay;  // Pot 4
    float noise_mix;    // Pot 5
    float env_curve;    // Pot 6
    float comp_amount;  // Pot 7
} WaveParams;

int waveform_generate(float* buffer, int max_samples, WaveParams* p);

#endif