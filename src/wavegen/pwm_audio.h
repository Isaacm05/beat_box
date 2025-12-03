#ifndef PWM_AUDIO_H
#define PWM_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_PIN 36
#define PWM_WRAP 255
#define PWM_DIV 1.0f

#define SAMPLE_RATE 22050.0f
#define MAX_SAMPLES 16384

void pwm_audio_init(void);
void pwm_play_pwm_nonblocking(const uint16_t* pwm_buffer, int len); // Direct PWM playback
bool pwm_is_playing(void);

extern uint16_t pwm_buf[MAX_SAMPLES];

#endif