// arduino_compat.h
#pragma once

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/timer.h"

// If not compiling in an Arduino environment, define minimal ARDUINO macros
#ifndef ARDUINO
#define ARDUINO 1
#endif

// Map pin modes
#ifndef INPUT
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#endif

static inline void pinMode(uint pin, int mode) {
    if (mode == OUTPUT) {
        gpio_set_dir(pin, GPIO_OUT);
    } else {
        // INPUT or INPUT_PULLUP
        gpio_set_dir(pin, GPIO_IN);
        if (mode == INPUT_PULLUP) {
            gpio_pull_up(pin);
        } else {
            gpio_disable_pulls(pin);
        }
    }
}

static inline void digitalWrite(uint pin, int val) {
    gpio_put(pin, val ? 1 : 0);
}

static inline int digitalRead(uint pin) {
    return gpio_get(pin);
}

// Timing functions — use pico SDK time functions.
// micros() returns lower 32 bits of microsecond counter, matching Arduino behavior.
static inline uint32_t micros(void) {
    // to_us_since_boot() returns uint64_t; mimic Arduino wrap-around by truncating to 32 bits
    return (uint32_t)(to_us_since_boot());
}

static inline uint32_t millis(void) {
    return (uint32_t)(to_ms_since_boot());
}

static inline void delayMicroseconds(uint32_t us) {
    sleep_us(us);
}

static inline void delay(uint32_t ms) {
    sleep_ms(ms);
}

// Attach/detach IRQ wrapper (if you need them)
static inline void attach_timer_irq(int irq, void (*handler)(void), bool enable) {
    // Not used directly; Protomatter uses irq_set_exclusive_handler etc directly.
    // Keep this for compatibility if some code calls it.
    irq_set_exclusive_handler(irq, (void (*)(void))handler);
    if (enable) irq_set_enabled(irq, true);
}

#endif // arduino_compat.h
