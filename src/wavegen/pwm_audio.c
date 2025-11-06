#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "waveform_gen.c"

#define AUDIO_PIN 15
#define SAMPLE_RATE 44100.0f

// Play a buffer through PWM
void pwm_play_buffer(float* buffer, int len) {
    uint slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    uint channel = pwm_gpio_to_channel(AUDIO_PIN);

    for (int i = 0; i < len; i++) {
        uint16_t level = (uint16_t) (buffer[i] * 65535);
        pwm_set_chan_level(slice, channel, level);
        sleep_us(1000000.0f / SAMPLE_RATE);
    }
}

// Initialize PWM output
void pwm_audio_init() {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(AUDIO_PIN);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f);
    pwm_init(slice, &config, true);
    pwm_set_wrap(slice, 65535);
}
