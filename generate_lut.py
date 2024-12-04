import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile
from scipy.ndimage import gaussian_filter1d

# Load the wav file
file_path = "phaser-impulse-3.wav"
sample_rate, data = wavfile.read(file_path)

# Make sure it's mono
if len(data.shape) > 1:
    data = data[0]

bpm = 1600
impulse_interval = 60 / bpm  # seconds per impulse
impulse_samples = int(impulse_interval * sample_rate)

num_impulses = len(data) // impulse_samples

# Extract impulses at equal intervals
impulses = [
    data[i * impulse_samples : (i + 1) * impulse_samples] for i in range(num_impulses)
]

# Frequency range of interest
freq_range = (350, 6500)

# Analyze each impulse
frequencies = None
magnitude_responses = []
lowest_frequencies = []

fft_resolution_factor=1
for impulse in impulses:
    # Zero-pad the signal to increase FFT resolution
    padded_length = len(impulse) * fft_resolution_factor
    spectrum = np.fft.rfft(impulse, n=padded_length)
    frequencies = np.fft.rfftfreq(padded_length, d=1 / sample_rate)
    magnitude = np.abs(spectrum)
    magnitude_responses.append(magnitude)

    # Find the frequency with the lowest magnitude in the range
    range_mask = (frequencies >= freq_range[0]) & (frequencies <= freq_range[1])
    freq_in_range = frequencies[range_mask]
    mag_in_range = magnitude[range_mask]
    lowest_freq = freq_in_range[np.argmin(mag_in_range)]
    lowest_frequencies.append(lowest_freq)

# # Plot impulses in groups of 10
# group_size = 10
# num_groups = (num_impulses + group_size - 1) // group_size  # ceil division
# 
# for group_idx in range(num_groups):
#     plt.figure(figsize=(12, 8))
#     for impulse_idx in range(group_size):
#         idx = group_idx * group_size + impulse_idx
#         if idx >= num_impulses:
#             break
#         plt.plot(
#             frequencies,
#             20 * np.log10(magnitude_responses[idx]),
#             label=f"Impulse {idx + 1}",
#             alpha=0.5,
#         )
# 
#     plt.xscale("log")
#     plt.xlim(freq_range)
#     plt.xlabel("Frequency (Hz)")
#     plt.ylabel("Magnitude (dB)")
#     plt.title(
#         f"Frequency Responses of Impulses {group_idx * group_size + 1} to "
#         f"{min((group_idx + 1) * group_size, num_impulses)}"
#     )
#     plt.grid(which="both", linestyle="--", linewidth=0.5)
#     plt.legend(loc="upper right", fontsize=8, ncol=2)
#     plt.tight_layout()
#     plt.show()

# Convert to angular frequencies and apply the scaling factor
wn_freqs = 2 * np.pi * np.array(lowest_frequencies)
wc_freqs = wn_freqs * (np.sqrt(2) - 1)

# Print the scaled angular frequencies
print("\nAngular Frequencies (radians/s):")
for idx, ang_freq in enumerate(wc_freqs):
    print(f"Impulse {idx + 1}: {ang_freq:.5f} radians/sample")

wc_freqs = gaussian_filter1d(wc_freqs, sigma=2)

plt.figure(figsize=(10, 6))
plt.plot(range(1, len(wc_freqs) + 1), wc_freqs, marker='o')
plt.xlabel("Impulse Index")
plt.ylabel("Angular Frequency")
plt.title("Angular Frequencies for Impulses")
plt.grid(True)
plt.tight_layout()
plt.show()

signal_half_period = wc_freqs[68:181]

# Mirror the signal to create a full period
signal_full_period = np.concatenate([signal_half_period, signal_half_period[::-1]])

plt.plot(np.linspace(0, 2 * np.pi, len(signal_full_period)), signal_full_period)
plt.xlabel("Phase (radians)")
plt.ylabel("Amplitude")
plt.title("Full Period Signal")
plt.grid(True)
plt.show()

lookup_table = signal_full_period.tolist()

# Print the lookup table in C/C++ array format
print("const float lookup_table[226] = {")
for i, value in enumerate(lookup_table):
    # Format the values as floats, with a comma after each one except the last
    if i < len(lookup_table) - 1:
        print(f"    {value:.6f},")
    else:
        print(f"    {value:.6f}")
print("};")
