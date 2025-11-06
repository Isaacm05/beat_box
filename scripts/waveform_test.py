import numpy as np
import matplotlib.pyplot as plt
from scipy.io.wavfile import write
from pathlib import Path

# --- Constants ---
SAMPLE_RATE = 44100.0
PI = np.pi


# --- Waveform Parameters ---
class WaveParams:
    def __init__(
        self,
        frequency,
        amplitude,
        decay,
        waveform_id,
        offset_dc,
        pitch_decay=0.0,
        noise_mix=0.0,
        env_curve=3.0,
        comp_amount=0.0,
    ):
        self.frequency = frequency
        self.amplitude = amplitude
        self.decay = decay
        self.waveform_id = waveform_id
        self.offset_dc = offset_dc
        self.pitch_decay = pitch_decay
        self.noise_mix = noise_mix
        self.env_curve = env_curve
        self.comp_amount = comp_amount


# --- Universal Waveform Generator ---
def waveform_generate(max_samples, p):
    total_samples = int(p.decay * SAMPLE_RATE)
    total_samples = min(total_samples, max_samples)
    t = np.arange(total_samples) / SAMPLE_RATE

    # --- Frequency envelope (pitch drop + drift) ---
    freq = (
        p.frequency
        * np.exp(-p.pitch_decay * t)
        * (1 + np.random.uniform(-0.002, 0.002))
    )
    phase = np.cumsum(freq / SAMPLE_RATE) % 1.0

    # --- Base waveform ---
    if p.waveform_id == 0:
        val = np.sin(2 * PI * phase)
    elif p.waveform_id == 1:
        val = np.where(phase < 0.5, 1.0, -1.0)
    elif p.waveform_id == 2:
        val = 4.0 * np.abs(phase - 0.5) - 1.0
    elif p.waveform_id == 3:
        val = 2.0 * phase - 1.0
    elif p.waveform_id == 4:
        val = np.random.uniform(-1.0, 1.0, total_samples)
    else:
        raise ValueError("Invalid waveform_id")

    # --- Noise blending ---
    if p.noise_mix > 0.0:
        noise = np.random.uniform(-1.0, 1.0, total_samples)
        val = (1 - p.noise_mix) * val + p.noise_mix * noise

    # --- Exponential amplitude envelope ---
    env = np.exp(-p.env_curve * t / p.decay)
    val = p.amplitude * env * val + p.offset_dc

    # --- Dynamic compression ---
    comp = np.clip(p.comp_amount, 0.0, 1.0)
    if comp > 0.0:
        threshold = 0.8 - 0.6 * comp
        if p.waveform_id == 0:
            ratio = 1.0 + 5.0 * comp
            makeup = 1.0 + 0.6 * comp
        else:
            ratio = 1.0 + 9.0 * comp
            makeup = 1.0 + 1.0 * comp

        abs_sig = np.abs(val)
        over = abs_sig > threshold 
        gain = np.ones_like(val)
        gain[over] = threshold + (abs_sig[over] - threshold) / ratio
        gain[over] /= abs_sig[over] + 1e-8
        val = val * gain * makeup

        if comp > 0.8:
            limit_level = 1.0 - (1.0 - threshold) * 0.5
            val = np.clip(val, -limit_level, limit_level)

    if p.waveform_id == 0:
        val *= 1.1

    return np.clip(val, -1.0, 1.0)


# --- Output Directory ---
root = Path(__file__).parents[1]
out_dir = root / "test" / "waveforms"
out_dir.mkdir(parents=True, exist_ok=True)


# --- Presets ---
presets = [
    WaveParams(
        60.0, 1.0, 0.25, 0, 0.0, pitch_decay=8.0, env_curve=4.0, comp_amount=0.5
    ),  # Kick
    WaveParams(
        250.0,
        0.8,
        0.15,
        0,
        0.0,
        pitch_decay=0.8,
        noise_mix=0.8,
        env_curve=5.0,
        comp_amount=0.6,
    ),  # Snare
    WaveParams(
        8000.0, 0.5, 0.05, 4, 0.0, noise_mix=1.0, env_curve=6.0, comp_amount=0.3
    ),  # Closed Hat
    WaveParams(
        55.0, 1.0, 1.2, 0, 0.0, pitch_decay=2.0, env_curve=2.5, comp_amount=0.25
    ),  # 808
    WaveParams(440.0, 0.7, 0.5, 2, 0.0, comp_amount=0.2),  # Tone
    WaveParams(
        8000.0, 0.5, 0.25, 4, 0.0, noise_mix=1.0, env_curve=3.0, comp_amount=0.4
    ),  # Open Hat
]

names = ["kick", "snare", "hat", "808", "tone", "open_hat"]


# --- Generate and Export ---
print("=== Waveform Generator Test ===")
for name, p in zip(names, presets):
    buffer = waveform_generate(int(SAMPLE_RATE), p)
    n = len(buffer)

    # Save WAV
    wav_path = out_dir / f"{name}.wav"
    scaled = np.int16(buffer * 32767)
    write(wav_path, int(SAMPLE_RATE), scaled)

    # Save Plot
    png_path = out_dir / f"{name}.png"
    plt.figure(figsize=(9, 3))
    plt.plot(buffer[:2000])
    plt.title(f"{name.capitalize()} Waveform Preview")
    plt.xlabel("Sample")
    plt.ylabel("Amplitude")
    plt.tight_layout()
    plt.savefig(png_path, dpi=150)
    plt.close()

    print(f"Saved {name}: {n} samples → {wav_path.name}, {png_path.name}")

print("All waveforms generated successfully.")
