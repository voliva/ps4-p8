#pragma once

#include <vector>;

unsigned int audio_get_wavelength(float frequency, int sample_rate);
unsigned int audio_get_points(float seconds, int sample_rate); // seconds => ticks

// Wave functions
// phase = 0.0->1.0, returns value between -1.0 and 1.0 representing that wave
float audio_sin_wave(float phase);
float audio_triangle_wave(float phase);
float audio_tilted_wave(float phase);
float audio_sawtooth_wave(float phase);
float audio_square_wave(float phase);
float audio_pulse_wave(float phase);
float audio_organ_wave(float phase);
float audio_noise_wave(float phase);
float audio_phaser_wave(float phase);

// Wave generators
void audio_generate_wave(float (*wave_fn)(float), unsigned int wavelength, std::vector<float>& dest);
void audio_generate_wave(float (*wave_fn)(float), unsigned int wavelength, std::vector<float>& dest, unsigned int from, unsigned int to);
void audio_smooth(std::vector<float>& dest, unsigned int position);
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