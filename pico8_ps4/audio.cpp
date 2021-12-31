#include "audio.h"
#include <math.h>
#include "log.h"

#define M_PI 3.14159265358979323

unsigned int audio_get_wavelength(float frequency) {
	return P8_SAMPLE_RATE / frequency;
}
unsigned int audio_get_points(float seconds) {
	return P8_SAMPLE_RATE * seconds;
}

float audio_sin_wave(float phase) {
	return sinf(phase * 2 * M_PI);
}
float audio_triangle_wave(float phase) {
	return fabs(phase - 0.5) * -4 + 1;
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

// #define SMOOTH_THRESHOLD 0.04 // Depends on sample rate :( - At 44100 for 130.81Hz breaks at 0.0005, but 0.01 is not noticable
void audio_generate_wave(float (*wave_fn)(float), unsigned int wavelength, std::vector<float>& dest, unsigned int from, unsigned int to) {
	int phase_shift = 0;
	// i => wave_fn((float)((i + phase_shift) % wavelength) / wavelength);

	if (from >= 2) {
		// Try and phase_shift until we match the previous value with a similar value+slope
		bool positive_slope = dest[from-1] >= dest[from-2];

		float smallest_diff = 1;
		int best_phase_shift = 0;
		for (phase_shift = 0; phase_shift < wavelength; phase_shift++) {
			float value = wave_fn((float)phase_shift / wavelength);

			bool would_be_positive_slope = wave_fn((float)(phase_shift +1) / wavelength) >= value;
			if (positive_slope == would_be_positive_slope) {
				float diff = fabs(value - dest[from - 1]);
				if (diff < smallest_diff) {
					best_phase_shift = phase_shift;
					smallest_diff = diff;
				}
			}
		}
		phase_shift = best_phase_shift;
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

// TODO
void audio_generate_noise(unsigned int wavelength, std::vector<float>& dest) {
	audio_generate_noise(wavelength, dest, 0, dest.size());
}
void audio_generate_noise(unsigned int wavelength, std::vector<float>& dest, unsigned int from, unsigned int to) {
	unsigned int len = to - from;

	for (int i = 0; i < len; i++) {
		dest[from + i] = 2.0 * rand() / RAND_MAX - 1.0;
	}

	/*int step = 10;
	float prev = 0;
	for (int s = 0; s <= len / step; s++) {
		float target = 2.0 * rand() / RAND_MAX - 1.0;
		for (int i = 0; i < step && (s*step + i) < len; i++) {
			dest[from + s * step + i] = prev + (i+1) * (target - prev) / step;
		}
		prev = target;
	}*/

	/*
	float f = 1;
	for (int i = 0; i < len; i++) {
		dest[from+i] = 2.0 * rand() / RAND_MAX - 1.0;
		if (i > 0) {
			dest[from + i] = (dest[from + i - 1] + f * (dest[from + i] - dest[from + i - 1])) / f;
		}
	}*/
}

// TODO
void audio_generate_phaser_wave(unsigned int wavelength, std::vector<float>& dest) {
	audio_generate_phaser_wave(wavelength, dest, 0, dest.size());
}
void audio_generate_phaser_wave(unsigned int wavelength, std::vector<float>& dest, unsigned int from, unsigned int to) {
	audio_generate_wave(audio_triangle_wave, wavelength, dest, from, to);
	/*audio_generate_wave(audio_triangle_wave, wavelength, dest, from, to);

	unsigned int len = to - from;
	for (unsigned int i = 0; i < len; i++) {
		float a_m = (cos(2 * M_PI * i / (90 * wavelength)) + 1) / 4 + 0.5;
		dest[from + i] = dest[from + i] * a_m;
	}*/
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

//// Pico8 ////

float base_frequencies[] = {
	65.40625, 69.295625, 73.41625, 77.781875, 82.406875, 87.3071875, 92.49875, 97.99875, 103.82625, 110, 116.5409375, 123.4709375
};
int multipliers[] = { 1, 2, 4, 8, 16, 32 };
float pitch_to_freq(unsigned char pitch) {
	int mul = pitch / 12;
	int base = pitch % 12;
	return base_frequencies[base] * multipliers[mul];
}

void audio_generate_note(P8_Note& note, std::vector<float>& dest, unsigned int from, unsigned int to) {
	float freq = pitch_to_freq(note.pitch);
	unsigned int wavelength = audio_get_wavelength(freq);
	switch (note.instrument) {
	case 0:
		return audio_generate_wave(audio_triangle_wave, wavelength, dest, from, to);
	case 1:
		return audio_generate_wave(audio_tilted_wave, wavelength, dest, from, to);
	case 2:
		return audio_generate_wave(audio_sawtooth_wave, wavelength, dest, from, to);
	case 3:
		return audio_generate_wave(audio_square_wave, wavelength, dest, from, to);
	case 4:
		return audio_generate_wave(audio_pulse_wave, wavelength, dest, from, to);
	case 5:
		return audio_generate_wave(audio_organ_wave, wavelength, dest, from, to);
	case 6:
		return audio_generate_noise(wavelength, dest, from, to);
	case 7:
		return audio_generate_phaser_wave(wavelength, dest, from, to);
	}
}

std::vector<int16_t> audio_buffer_from_sfx(P8_SFX &sfx) {
	int num_notes = 32;
	while (num_notes > 0 && sfx.notes[num_notes - 1].volume == 0) {
		num_notes--;
	}
	int speed = sfx.speed;
	if (speed < 1) {
		speed = 1;
	}

	int total_ticks = speed * P8_TICKS_PER_T * num_notes;
	std::vector<float> floats(total_ticks);
	for (int i = 0; i < num_notes; i++) {
		if (sfx.notes[i].volume > 0) {
			audio_generate_note(sfx.notes[i], floats, speed * P8_TICKS_PER_T * i, speed * P8_TICKS_PER_T * (i + 1));
		}
	}

	std::vector<int16_t> result(total_ticks);
	for (int n = 0; n < num_notes; n++) {
		if (sfx.notes[n].volume > 0) {
			for (int i = 0; i < speed * P8_TICKS_PER_T; i++) {
				result[n * speed * P8_TICKS_PER_T + i] = sfx.notes[n].volume * floats[n * speed * P8_TICKS_PER_T + i] * 2000;
			}
		}
	}

	return result;
}
