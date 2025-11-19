#ifndef ADC_POTENTIOMETER_H
#define ADC_POTENTIOMETER_H

#include "../wavegen/waveform_gen.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Set up pot and button io/configs
// Note all pins may be subject to change, so these values are not finalized

#define BUTTON_PIN 21 // Pin num of button
#define POT_PIN 44    // Pin num of 1st potentiometer, im assuming follow sequentially
#define POT_NUM 4     // Number of potentiometers
#define PARAM_NUM 8   // Number of parameters, 8 as we know of rn

extern volatile bool mode_flag; // false = page 0 (params 0-3), true = page 1 (params 4-7)

void init_button(void);
void init_adc_dma(void);
void get_pot_val(void);
WaveParams normal_pots(WaveParams adc_buffer);

#endif