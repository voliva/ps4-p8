#include "audio.h"
#include <math.h>
#include "log.h"

#define M_PI 3.14159265358979323

unsigned int audio_get_wavelength(float frequency, int sample_rate) {
	return sample_rate / frequency;
}
unsigned int audio_get_points(float seconds, int sample_rate) {
	return sample_rate * seconds;
}

float audio_sin_wave(float phase) {
	return sinf(phase * 2 * M_PI);
}
float audio_triangle_wave(float phase) {
	return abs(phase - 0.5) * -4 + 1;
}

#define TILTED_TIP 0.9
float audio_tilted_wave(float phase) {
	if (phase < TILTED_TIP) {
		return (phase / TILTED_TIP) * 2 - 1;
	}
	else {
		return (1 + 2 * TILTED_TIP / (1-TILTED_TIP)) - (phase / (1 - TILTED_TIP)) * 2;
	}
}
float audio_sawtooth_wave(float phase) {
	return phase * 2 - 1;
}
float audio_square_wave(float phase) {
	if (phase < 0.5) {
		return 0;
	}
	else {
		return 1;
	}
}
float audio_pulse_wave(float phase) {
	if (phase < 0.75) {
		return 0;
	}
	else {
		return 1;
	}
}
float audio_organ_wave(float phase) {
	return (audio_triangle_wave(phase) + audio_triangle_wave(fmod(phase * 2, 1))) / 2;
}
float audio_noise_wave(float phase) {
	return ((float)rand() / RAND_MAX * 2) - 1;
}

void audio_generate_wave(float (*wave_fn)(float), unsigned int wavelength, std::vector<float>& dest) {
	audio_generate_wave(wave_fn, wavelength, dest, 0, dest.size());
}

#define SMOOTH_THRESHOLD 0.01 // Depends on sample rate :( - At 44100 for 130.81Hz breaks at 0.0005, but 0.01 is not noticable
void audio_generate_wave(float (*wave_fn)(float), unsigned int wavelength, std::vector<float>& dest, unsigned int from, unsigned int to) {
	int phase_shift = 0;
	// i => wave_fn((float)((i + phase_shift) % wavelength) / wavelength);

	if (from >= 2) {
		// Try and phase_shift until we match the previous value with a similar value+slope
		bool positive_slope = dest[from-1] >= dest[from-2];

		for (phase_shift = 0; phase_shift < wavelength; phase_shift++) {
			float value = wave_fn((float)phase_shift / wavelength);
			if (fabs(value - dest[from - 1]) <= SMOOTH_THRESHOLD) {
				bool would_be_positive_slope = wave_fn((float)(phase_shift +1) / wavelength) >= value;
				if (positive_slope == would_be_positive_slope) {
					break;
				}
			}
		}
	}

	unsigned int len = to - from;
	// Generate the first wave
	for (unsigned int i = 0; i < wavelength && i < len; i++) {
		dest[from + i] = wave_fn((float)((i + phase_shift) % wavelength) / wavelength);
	}

	// Copy it until we reach the end
	for (unsigned int i = wavelength; i < len; i++) {
		dest[from + i] = dest[from + (i % wavelength)];
	}
}
void audio_generate_phaser_wave(unsigned int wavelength, std::vector<float>& dest) {
	audio_generate_phaser_wave(wavelength, dest, 0, dest.size());
}
void audio_generate_phaser_wave(unsigned int wavelength, std::vector<float>& dest, unsigned int from, unsigned int to) {
	unsigned int len = to - from;
	// Generate the first wave
	for (unsigned int i = 0; i < wavelength && i < len; i++) {
		dest[from + i] = audio_triangle_wave((float)i / wavelength);
	}
	// Copy it until we reach the end
	for (unsigned int i = wavelength; i < len; i++) {
		float a_m = (cos(i / 90 * 2 * M_PI) + 1) / 4 + 0.5;
		dest[from + i] = dest[from + i % wavelength] * a_m;
	}
}

void audio_amplify(std::vector<float>& src, std::vector<int16_t>& dest, unsigned int volume) {
	audio_amplify(src, dest, volume, 0);
}
void audio_amplify(std::vector<float>& src, std::vector<int16_t>& dest, unsigned int volume, unsigned int from) {
	int max = dest.size() - from;
	if (max > src.size()) {
		max = src.size();
	}
	for (int i = 0; i < max; i++) {
		dest[i+from] = volume * src[i];
	}
}
