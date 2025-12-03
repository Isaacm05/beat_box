#ifndef WAVEFORM_GEN_H
#define WAVEFORM_GEN_H

#include <stdint.h>

typedef struct {
    float frequency; // Pot 0            20-9kHz
    float amplitude; // Pot 1            0.0-1.0
    float decay;     // Pot 2            0.0-2.0s
    int waveform_id; // 0=sine, 1=square, 2=triangle, 3=saw, 4=noise
    float offset_dc; // Pot 3            0.0-1.0

    float pitch_decay; // Pot 4            0.0-10.0
    float noise_mix;   // Pot 5            0.0-1.0
    float env_curve;   // Pot 6            0.0-10.0
    float comp_amount; // Pot 7            0.0-1.0
} WaveParams;

// void waveform_init(void);

// Legacy function - generates float samples
int waveform_generate(float* buffer, int max_samples, WaveParams* p);

// New memory-optimized function - generates PWM values directly
int waveform_generate_pwm(uint16_t* pwm_buffer, int max_samples, WaveParams* p);

#endif