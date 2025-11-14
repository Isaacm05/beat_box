#include "pwm_audio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>

// ==================================================
// INIT PWM FOR AUDIO
// ==================================================
void pwm_audio_init() {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    int slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    int channel = pwm_gpio_to_channel(AUDIO_PIN);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, PWM_DIV);
    pwm_config_set_wrap(&cfg, PWM_WRAP);

    pwm_init(slice, &cfg, true);
    pwm_set_chan_level(slice, channel, 0);
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

void pwm_play_buffer(const float* buffer, int len) {
    uint16_t pwm_buf[(int) SAMPLE_RATE];
    convert_float_to_pwm(buffer, pwm_buf, len);

    printf("buffer[0] = %u\n", pwm_buf[0]);
    printf("buffer[100] = %u\n", pwm_buf[100]);
    printf("buffer[500] = %u\n", pwm_buf[500]);

    int slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    int channel = pwm_gpio_to_channel(AUDIO_PIN);

    const uint32_t sample_delay_us = (uint32_t) (1e6f / SAMPLE_RATE);

    for (int i = 0; i < len; i++) {
        pwm_set_chan_level(slice, channel, pwm_buf[i]);
        sleep_us(sample_delay_us);
    }
}
