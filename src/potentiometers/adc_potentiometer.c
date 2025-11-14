#include "adc_potentiometer.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Global variable definitions

uint16_t raw_adc_buffer[PARAM_NUM];
float unnorm_buffer[PARAM_NUM];
volatile bool mode_flag = false;
bool pot_engaged[4] = {false};
bool last_mode = false;

/*

Explaining thought process for the 4 pots and button switch up.

Basically we have 8 parameters to control, but only 4 pots.
So we can use a button to switch between 2 different "modes"/"pages"(in the ui sense)
each mode/page will have 4 pots mapped to their own parameter.

My thinking is that when the button is pressed, we toggle where the memory for each of these pots
goes to. My only issue would be, when switching modes, rather than remembering what was the previous
set value for the param, we would see the params jump to the current pot values, which were adjusted
for the earlier parameter.

To read from the different modes, im thinking we have the button press toggle,
basically either send an interrupt to set a flag high or low, just toggle it for the 2 pages of
params we have I dont think we have 2 FIFO's for the DMA and I dont think there's a way to configure
it like that in hardware

So somewhere I software I need to make it so the flag that is toggled by the button makes the pots
write to different arrays/banks.

so like bank/array 0 would be for params 0-3
and bank/array 1 would be for params 4-7

Going to check this w TA to see if it makes sense, if somebody is reviewing this
and has suggestions or comments please lmk
*/

// ISR for button press, if acknowledged, toggle the mode/page flag
void button_isr() {

    uint32_t evmask = gpio_get_irq_event_mask(BUTTON_PIN);
    if (evmask) {
        gpio_acknowledge_irq(BUTTON_PIN, GPIO_IRQ_EDGE_RISE);

        // Toggle mode flag
        mode_flag = !mode_flag;
    }
}

// P easy to see what this function does lol
void init_button() {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);

    uint32_t mask = 1u << BUTTON_PIN;

    gpio_add_raw_irq_handler_masked(mask, button_isr);
    gpio_set_irq_enabled(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void init_adc_dma() {
    adc_init();

    for (int i = 0; i < POT_NUM; i++) {
        adc_gpio_init(POT_PIN + i);
    }

    // 0x0F is 0b00001111, enabling channels 0-3
    adc_set_round_robin(0x0F); // Enable channels 0-3 for round robin
    adc_fifo_setup(true,       // Write each completed conversion to the FIFO
                   true,       // Enable DMA data request (DREQ)
                   POT_NUM,    // DREQ (and IRQ) asserted when at least POT_NUM samples present
                   false,      // No ERR bit
                   false       // No 8-bit mode
    );

    adc_run(true); // Start ADC conversions

    int dma_chan = 0;
    dma_hw->ch[dma_chan].transfer_count = (1u << 28) | POT_NUM;
    dma_hw->ch[dma_chan].read_addr = (uint32_t)&adc_hw->fifo;
    dma_hw->ch[dma_chan].write_addr = (uint32_t)raw_adc_buffer;

    uint32_t temp = 0;
    temp |= (1u << 2);
    temp |= (DREQ_ADC << 17);
    temp |= (1u << 0);
    dma_hw->ch[dma_chan].ctrl_trig = temp;
}

/*Current thoughts on design, first must test to see if will work on board.
 - Make sure all pots are wired to board, then test values.

When integrating with the top lvl, make sure to read the adc_buffer values based on the mode_flag
 - If mode_flag is false, read adc_buffer[0-3] to params 0-3
 - If mode_flag is true, read adc_buffer[0-3] to params 4-7
This way we can switch between 2 sets of parameters using the button

Changes/Updates that could be made:
 - Instead of using a single adc_buffer, could use 2 separate buffers for each mode/page
   This would allow us to remember the last set values for each parameter when switching modes
   But requires more memory
 - Could implement debouncing for the button press to avoid multiple toggles on a single press

 Oh yea need to add functionality for memory later, so that when switching modes, we can remember
    the last set values for each parameter, and only change them when the pots are adjusted from
their last positions Annoying cause when switching pages the pot position will jump to the current
pot values, which may not be desired

    Fix would be to only update the parameter values when the pot values change significantly
    like a locker combo, you have to turn the knob back past 50% or smth. (Our pots arent always
accurate so 50% may be different for each one)
*/

void get_pot_val() {

    if (mode_flag != last_mode) {
        for (int i = 0; i < 4; i++)
            pot_engaged[i] = false;
        last_mode = mode_flag;
    }

    for (int i = 0; i < 4; i++) {

        float pot_val = raw_adc_buffer[i] / 4095.0f;
        int idx = i + (mode_flag ? 4 : 0);

        float param_val = unnorm_buffer[idx];

        if (!pot_engaged[i]) {
            if (fabs(pot_val - param_val) <= 0.02) {
                pot_engaged[i] = true;
            } else {
                continue;
            }
        }

        unnorm_buffer[idx] = pot_val;
    }

    if (mode_flag) {
        // Map to parameters 4-7
        for (int i = 0; i < POT_NUM; i++) {
            printf("Param %d: %f\n", i + POT_NUM, unnorm_buffer[i + POT_NUM]);
        }
    } else {
        // Map to parameters 0-3
        for (int i = 0; i < POT_NUM; i++) {
            printf("Param %d: %f\n", i, unnorm_buffer[i]);
        }
    }
}

WaveParams normal_pots(WaveParams adc_buffer) {
    WaveParams params = adc_buffer;

    // Pot 0: Frequency (20-9000 Hz)
    params.frequency = unnorm_buffer[0] * (9000.0f - 20.0f) + 20.0f;

    // Pot 1: Amplitude (0.0-1.0)
    params.amplitude = unnorm_buffer[1];

    // Pot 2: Decay (0.0-2.0 s)
    params.decay = unnorm_buffer[2] * 2.0f;

    // Pot 3: DC Offset (0.0-1.0)
    params.offset_dc = unnorm_buffer[3];

    // Pot 4: Pitch Decay (0.0-10.0) - logarithmic mapping for better control
    // Maps pot value (0.0-1.0) logarithmically to range (0.0-10.0)
    // Using exp(x) - 1 to get exponential curve from 0 to ~1.7, then scale to 10
    params.pitch_decay = (expf(unnorm_buffer[4] * 2.3026f) - 1.0f) * 1.0f;

    // Pot 5: Noise Mix (0.0-1.0)
    params.noise_mix = unnorm_buffer[5];

    // Pot 6: Envelope Curve (0.0-10.0)
    params.env_curve = unnorm_buffer[6] * 10.0f;

    // Pot 7: Compression Amount (0.0-1.0)
    params.comp_amount = unnorm_buffer[7];

    return params;
}

// Main function
/*int main() {
    stdio_init_all();

    init_button();
    init_adc_dma();

    while (1) {
        // Main loop can be used to process adc_buffer based on mode_flag
        // For example, map adc_buffer values to parameters based on mode_flag

        check_pots();

        get_pots();

        sleep_ms(500); // Adjust as needed
    }

    return 0;
}
*/