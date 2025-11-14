#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include <stdio.h>
#include <stdlib.h>

// Set up pot and button io/configs
// Note all pins may be subject to change, so these values are not finalzied

#define BUTTON_PIN 15 // Pin num of button
#define POT_PIN 26    // Pin num of 1st potentiometer, im assuming follow sequentially
#define POT_NUM 4     // Number of potentiometers
#define PARAM_NUM 8   // Number of parameters, 8 as we know of rn

extern uint16_t raw_adc_buffer[PARAM_NUM];
extern float adc_buffer[PARAM_NUM];

extern volatile bool mode_flag; // false = page 0 (params 0-3), true = page 1 (params 4-7)

extern bool pot_engaged[4];
extern bool last_mode;

void init_button();
void init_adc_dma();
void check_pots();