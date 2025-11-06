#include "waveform_gen.c"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== Waveform Generator ===\n");

    float buffer[(int) SAMPLE_RATE];

    WaveParams presets[] = {
        {60.0f, 1.0f, 0.25f, 0, 0.0f, 8.0f, 0.0f, 4.0f, 0.5f},   // Kick
        {250.0f, 0.8f, 0.15f, 0, 0.0f, 0.8f, 0.8f, 5.0f, 0.6f},  // Snare
        {8000.0f, 0.5f, 0.05f, 4, 0.0f, 0.0f, 1.0f, 6.0f, 0.3f}, // Hat
        {55.0f, 1.0f, 1.2f, 0, 0.0f, 2.0f, 0.0f, 2.5f, 0.25f},   // 808
        {440.0f, 0.7f, 0.5f, 2, 0.0f, 0.0f, 0.0f, 3.0f, 0.2f},   // Tone
        {8000.0f, 0.5f, 0.25f, 4, 0.0f, 0.0f, 1.0f, 3.0f, 0.4f}  // Open hat
    };

    const char* names[] = {"kick", "snare", "hat", "808", "tone", "open_hat"};

    for (int i = 0; i < 6; i++) {
        int n = waveform_generate(buffer, SAMPLE_RATE, &presets[i]);
        printf("Generated %s (%d samples)\n", names[i], n);
    }

    printf("All waveforms generated successfully.\n");
    return 0;
}
