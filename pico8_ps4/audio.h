#pragma once

#include <vector>;

#define P8_SAMPLE_RATE 22050

unsigned int audio_get_wavelength(float frequency);
unsigned int audio_get_points(float seconds); // seconds => ticks

// Wave functions
// phase = 0.0->1.0, returns value between -1.0 and 1.0 representing that wave
float audio_sin_wave(float phase);
float audio_triangle_wave(float phase);
float audio_tilted_wave(float phase);
float audio_sawtooth_wave(float phase);
float audio_square_wave(float phase);
float audio_pulse_wave(float phase);
float audio_organ_wave(float phase);

// Wave generators
void audio_generate_wave(float (*wave_fn)(float), unsigned int wavelength, std::vector<float>& dest);
void audio_generate_wave(float (*wave_fn)(float), unsigned int wavelength, std::vector<float>& dest, unsigned int from, unsigned int to);
void audio_generate_noise(unsigned int wavelength, std::vector<float>& dest);
void audio_generate_noise(unsigned int wavelength, std::vector<float>& dest, unsigned int from, unsigned int to);
void audio_generate_phaser_wave(unsigned int wavelength, std::vector<float>& dest);
void audio_generate_phaser_wave(unsigned int wavelength, std::vector<float>& dest, unsigned int from, unsigned int to);
void audio_amplify(std::vector<float>& src, std::vector<int16_t>& dest, unsigned int volume);
void audio_amplify(std::vector<float>& src, std::vector<int16_t>& dest, unsigned int volume, unsigned int from);

// Effects
// void audio_effect_fadein(std::vector<float>& wave);
// void audio_effect_fadeout(std::vector<float>& wave);

// Filters => How to do them? I don't want to do FFT :'D
// Plus these
//void audio_effect_slide(std::vector<float>& wave);
//void audio_eaffect_vibrato(std::vector<float>& wave);
//void audio_effect_drop(std::vector<float>& wave);

typedef struct {
	unsigned char instrument; // [0-7 default instruments, 8-15 custom instruments]
	unsigned char volume; // [0-7]
	unsigned char effect; // [0-7]
	unsigned char pitch; // [0-63]
} P8_Note;

#define P8_TICKS_PER_T 183
typedef struct {
	bool noiz;
	bool buzz;
	char detune;
	char reverb;
	char dampen;
	char speed;
	char loopStart;
	char loopEnd;
	P8_Note notes[32];
} P8_SFX;

std::vector<int16_t> audio_buffer_from_sfx(P8_SFX &sfx);