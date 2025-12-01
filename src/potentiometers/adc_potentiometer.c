#include "adc_potentiometer.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include <stddef.h>

// Define global variables
volatile uint32_t raw_adc_val;
volatile int idx = 0;
volatile bool update_lcd_params = 0;
bool pot_engaged[PARAM_NUM] = {false}; // Explicitly initialize all to false
WaveParams* current_params;

// this is for when we have sets of params
void set_current_params(WaveParams* params) {
    current_params = params;
}

void button_isr_left() {

    uint32_t evmask = gpio_get_irq_event_mask(BUTTON_PIN_LEFT);
    if (evmask) {
        gpio_acknowledge_irq(BUTTON_PIN_LEFT, GPIO_IRQ_EDGE_RISE);

        // Toggle mode flag
        if (idx == 0 || idx == 3 || idx == 4 || idx == 7) {
            update_lcd_params = true;
        }
        idx--;
        idx = (idx < 0) ? (PARAM_NUM - 1) : idx;

        // Mark new parameter as not engaged yet
        pot_engaged[idx] = false;
    }
}

void button_isr_right() {

    uint32_t evmask = gpio_get_irq_event_mask(BUTTON_PIN_RIGHT);
    if (evmask) {
        gpio_acknowledge_irq(BUTTON_PIN_RIGHT, GPIO_IRQ_EDGE_RISE);

        // Toggle mode flag
        if (idx == 0 || idx == 3 || idx == 4 || idx == 7) {
            update_lcd_params = true;
        }
        idx++;
        idx = (idx >= PARAM_NUM) ? 0 : idx;

        // Mark new parameter as not engaged yet
        pot_engaged[idx] = false;
    }
}

// P easy to see what this function does lol
void init_button(int button_pin) {
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_IN);

    uint32_t mask = 1u << button_pin;

    if (button_pin == BUTTON_PIN_LEFT) {
        gpio_add_raw_irq_handler_masked(mask, button_isr_left);
    } else {
        gpio_add_raw_irq_handler_masked(mask, button_isr_right);
    }
    gpio_set_irq_enabled(button_pin, GPIO_IRQ_EDGE_RISE, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void init_adc() {
    // fill in
    adc_gpio_init(POT_PIN);
    adc_init();
    adc_select_input(4);
}

void init_adc_freerun() {
    // fill in
    init_adc();
    adc_run(true);
}

void init_dma() {
    // fill in
    int dma_chan = dma_claim_unused_channel(true);
    dma_hw->ch[dma_chan].transfer_count = (1u << 28) | 1u;

    uint32_t temp = 0;
    temp |= (1u << 2);
    temp |= (DREQ_ADC << 17);
    temp |= 1u << 0;
    dma_hw->ch[dma_chan].read_addr = (uint32_t) &adc_hw->fifo;
    dma_hw->ch[dma_chan].write_addr = (uint32_t) &raw_adc_val;
    dma_hw->ch[dma_chan].ctrl_trig |= temp;
}

void init_adc_dma() {
    // fill in
    init_dma();
    init_adc_freerun();
    adc_hw->fcs |= ADC_FCS_EN_BITS;
    adc_hw->fcs |= ADC_FCS_DREQ_EN_BITS;
}

bool update_pots(WaveParams* params) {

    if (idx >= PARAM_NUM)
        return false;

    const float pot_val = raw_adc_val * (1.0f / 4095.0f);
    const typeof(param_config[0])* cfg = &param_config[idx];
    float* param_ptr = (float*) ((uint8_t*) params + cfg->offset);

    // Normalize
    float param_normalized;
    if (cfg->is_exponential) {
        // Inverse of exponential scale
        float ratio = cfg->max_val / cfg->min_val;
        param_normalized = logf(*param_ptr / cfg->min_val) / logf(ratio);
    } else {
        param_normalized = (*param_ptr - cfg->min_val) / (cfg->max_val - cfg->min_val);
    }

    // Check engagement
    if (!pot_engaged[idx]) {
        if (fabsf(pot_val - param_normalized) <= POT_ENGAGE_THRESHOLD) {
            pot_engaged[idx] = true;
        } else {
            return false; // Not engaged yet, don't update
        }
    }

    // Calculate new parameter value
    float new_value;
    if (cfg->is_exponential) {
        // Exponential scaling
        float ratio = cfg->max_val / cfg->min_val;
        new_value = cfg->min_val * expf(pot_val * logf(ratio));
    } else {
        new_value = pot_val * (cfg->max_val - cfg->min_val) + cfg->min_val;
    }

    // Check if value changed
    if (fabsf(new_value - *param_ptr) <= cfg->threshold) {
        return false;
    }

    *param_ptr = new_value;
    return true;
}

// // Main function
// int main() {
//     stdio_init_all();

//     init_button(BUTTON_PIN_LEFT);
//     init_button(BUTTON_PIN_RIGHT);

//     init_adc_dma();
//     while (1) {
//         // Main loop can be used to process adc_buffer based on mode_flag
//         // For example, map adc_buffer values to parameters based on mode_flag
//         pot_buff[idx] = raw_adc_val;
//         printf("Param %d: %d\n", idx, pot_buff[idx]);

//         printf("\n");

//         /* If wanted to print all params each time to see if being stored properly
//         printf("Param 0: %d, Param 1: %d, Param 2: %d, Param 3: %d, Param 4: %d, Param 5: %d,
//         Param 6: %d, Param 7: %d\n", pot_buff[0], pot_buff[1], pot_buff[2], pot_buff[3],
//         pot_buff[4], pot_buff[5], pot_buff[6], pot_buff[7]);
//         */
//         sleep_ms(500); // Adjust as needed
//     }
//     return 0;
// }