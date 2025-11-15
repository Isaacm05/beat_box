#ifndef PWM_AUDIO_H
#define PWM_AUDIO_H

#include <stdint.h>

#define AUDIO_PIN 36
#define PWM_WRAP 255
#define PWM_DIV 1.0f // no clock divide
#define SAMPLE_RATE 44100.0f

void pwm_audio_init(void);
void pwm_play_buffer(const float* buffer, int len);

#endif