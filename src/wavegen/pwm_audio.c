#include "pwm_audio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>

// Global PWM buffer
uint16_t pwm_buf[MAX_SAMPLES];

static int dma_chan = -1;
static int pwm_slice;
static int pwm_channel;
static volatile bool is_playing = false;

void dma_irq_handler() {
    if (dma_channel_get_irq0_status(dma_chan)) {
        dma_channel_acknowledge_irq0(dma_chan);
        is_playing = false;
    }
}

void pwm_audio_init() {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    pwm_slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    pwm_channel = pwm_gpio_to_channel(AUDIO_PIN);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, PWM_DIV);
    pwm_config_set_wrap(&cfg, PWM_WRAP);

    pwm_init(pwm_slice, &cfg, true);
    pwm_set_chan_level(pwm_slice, pwm_channel, 0);

    dma_chan = dma_claim_unused_channel(true);

    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

// NO CONVERSION USE THIS ONE
void pwm_play_pwm_nonblocking(const uint16_t* pwm_buffer, int len) {

    if (is_playing) {
        return;
    }

    is_playing = true;

    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);

    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);

    channel_config_set_dreq(&cfg, pwm_get_dreq(pwm_slice));

    volatile void* pwm_cc_reg = &pwm_hw->slice[pwm_slice].cc;
    volatile uint16_t* pwm_output_reg = (volatile uint16_t*) pwm_cc_reg + pwm_channel;

    dma_channel_configure(dma_chan, &cfg, pwm_output_reg, pwm_buffer, len, false);

    float sys_clk = 125000000.0f;
    float pwm_freq = SAMPLE_RATE;
    float cycles_per_sample = sys_clk / pwm_freq;

    pwm_config cfg_pwm = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg_pwm, cycles_per_sample / (PWM_WRAP + 1));
    pwm_config_set_wrap(&cfg_pwm, PWM_WRAP);
    pwm_init(pwm_slice, &cfg_pwm, true);

    dma_channel_start(dma_chan);
}

// Check if audio is currently playing
bool pwm_is_playing(void) {
    return is_playing;
}
