#ifndef ADC_POTENTIOMETER_H
#define ADC_POTENTIOMETER_H

#include "../wavegen/waveform_gen.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Lookup table for parameter configuration (stored in flash, not RAM)
static const struct {
    uint16_t offset;        // Byte offset of parameter in WaveParams struct
    float min_val;          // Minimum parameter value
    float max_val;          // Maximum parameter value
    float threshold;        // Change threshold for LCD update
    uint8_t is_exponential; // 1 if exponential scaling, 0 if linear
} param_config[8] = {
    {offsetof(WaveParams, frequency), 20.0f, 9000.0f, 75.0f, 1}, // 0: Frequency
    {offsetof(WaveParams, amplitude), 0.0f, 1.0f, 0.01f, 0},     // 1: Amplitude
    {offsetof(WaveParams, decay), 0.0f, 2.0f, 0.01f, 0},         // 2: Decay
    {offsetof(WaveParams, offset_dc), 0.0f, 1.0f, 0.01f, 0},     // 3: DC Offset
    {offsetof(WaveParams, pitch_decay), 0.0f, 10.0f, 0.1f, 1},   // 4: Pitch Decay (exp)
    {offsetof(WaveParams, noise_mix), 0.0f, 1.0f, 0.1f, 0},      // 5: Noise Mix
    {offsetof(WaveParams, env_curve), 0.0f, 10.0f, 0.1f, 0},     // 6: Envelope Curve
    {offsetof(WaveParams, comp_amount), 0.0f, 1.0f, 0.01f, 0}    // 7: Compression
};

// Set up pot and button io/configs
// Note all pins may be subject to change, so these values are not finalized

#define BUTTON_PIN_LEFT 21         // Pin num of button left
#define BUTTON_PIN_RIGHT 26        // Pin num of button right
#define POT_PIN 44                 // Pin num of ptentiomeer
#define PARAM_NUM 8                // Number of parameters, 8 as we know of rn
#define POT_ENGAGE_THRESHOLD 0.05f // Engagement threshold

extern volatile uint32_t raw_adc_val;
extern volatile int idx;            // Right = ++/direc = true, Left = --/direc = false
extern bool pot_engaged[PARAM_NUM]; // Track if pot is engaged for each parameter
extern WaveParams* current_params;  // Pointer to current WaveParams being controlled

void init_dma();
void init_adc_dma();
void init_adc_freerun();
void init_adc();
void init_button(int button_pin);
void button_isr_right();
void button_isr_left();
bool update_pots(WaveParams* params);
void set_current_params(WaveParams* params);

#endif