#include "pwm_audio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>

// Global PWM buffer (made global for debugging access)
// Memory optimized: 16384 samples * 2 bytes = 32KB
uint16_t pwm_buf[MAX_SAMPLES];

// DMA channel for audio playback
static int dma_chan = -1;
static int pwm_slice;
static int pwm_channel;
static volatile bool is_playing = false;

// DMA interrupt handler - called when playback completes
void dma_irq_handler() {
    if (dma_channel_get_irq0_status(dma_chan)) {
        dma_channel_acknowledge_irq0(dma_chan);
        is_playing = false;
    }
}

// ==================================================
// INIT PWM FOR AUDIO
// ==================================================
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

    // Set up DMA IRQ
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void convert_float_to_pwm(const float* float_buf, uint16_t* pwm_buf, int len) {
    for (int i = 0; i < len; i++) {
        float x = float_buf[i];

        // clamp
        if (x < -1.0f)
            x = -1.0f;
        if (x > 1.0f)
            x = 1.0f;

        float normalized = (x + 1.0f) * 0.5f; // now 0..1
        pwm_buf[i] = (uint16_t) (normalized * PWM_WRAP);
    }
}

// Blocking version (original implementation - kept for compatibility)
void pwm_play_buffer(const float* buffer, int len) {
    convert_float_to_pwm(buffer, pwm_buf, len);

    const uint32_t sample_delay_us = (uint32_t) (1e6f / SAMPLE_RATE);

    for (int i = 0; i < len; i++) {
        pwm_set_chan_level(pwm_slice, pwm_channel, pwm_buf[i]);
        sleep_us(sample_delay_us);
    }
}

// Non-blocking version using DMA and hardware timer
void pwm_play_buffer_nonblocking(const float* buffer, int len) {
    // Don't start new playback if already playing
    if (is_playing) {
        return;
    }

    // Convert float samples to PWM values
    convert_float_to_pwm(buffer, pwm_buf, len);

    is_playing = true;

    // Configure DMA channel
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);

    // Transfer uint16_t values from memory to PWM
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, true);  // Read from array
    channel_config_set_write_increment(&cfg, false); // Always write to same PWM register

    // Set up pacing based on PWM slice
    channel_config_set_dreq(&cfg, pwm_get_dreq(pwm_slice));

    // Get the address of the PWM counter compare register
    volatile void* pwm_cc_reg = &pwm_hw->slice[pwm_slice].cc;
    // Offset to the correct channel (A=0, B=2 bytes)
    volatile uint16_t* pwm_output_reg = (volatile uint16_t*) pwm_cc_reg + pwm_channel;

    // Configure the DMA transfer
    dma_channel_configure(dma_chan, &cfg, pwm_output_reg, // Write to PWM
                          pwm_buf,                        // Read from buffer
                          len,                            // Number of transfers
                          false                           // Don't start yet
    );

    // Set PWM frequency to match sample rate
    // PWM runs at system clock / (div * (wrap+1))
    // We want PWM to wrap at sample_rate Hz
    float sys_clk = 125000000.0f; // 125 MHz system clock
    float pwm_freq = SAMPLE_RATE;
    float cycles_per_sample = sys_clk / pwm_freq;

    pwm_config cfg_pwm = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg_pwm, cycles_per_sample / (PWM_WRAP + 1));
    pwm_config_set_wrap(&cfg_pwm, PWM_WRAP);
    pwm_init(pwm_slice, &cfg_pwm, true);

    // Start the DMA transfer
    dma_channel_start(dma_chan);
}

// Check if audio is currently playing
bool pwm_is_playing(void) {
    return is_playing;
}
