#include "audio.h"
#include <math.h>
#include "log.h"
#include "memory.h"

#define M_PI 3.14159265358979323

#define DEBUGLOG Audio_DEBUGLOG
Log DEBUGLOG = logger.log("audio");

void audio_cb(void* userdata, Uint8* stream, int len);

// You can hear a "heartbeat"
//#define STEP 0.01
//void smooth(std::vector<float>& f, int pos) {
//	float prev = f[pos - 1];
//	for (int i = pos; fabs(f[i] - prev) > 0.01; i++) {
//		if (prev < f[i]) {
//			f[i] = prev - STEP;
//		} else {
//			f[i] = prev + STEP;
//		}
//	}
//}

#define CROSS_FADE 2000
AudioManager::AudioManager()
{
	for (int i = 0; i < SFX_AMOUNT; i++) {
		this->cache[i] = NULL;
	}

	for (int i = 0; i < CHANNELS; i++) {
		SDL_AudioSpec* audio_spec = new SDL_AudioSpec;
		// SDL_zero(audio_spec);
		audio_spec->freq = P8_SAMPLE_RATE;
		audio_spec->format = AUDIO_F32LSB;
		audio_spec->channels = 1; // mono/stereo/quad/5.1
		audio_spec->samples = 1024;
		audio_spec->callback = NULL;
		audio_spec->userdata = &this->channels[i];

		this->channels[i].spec = audio_spec;
		this->channels[i].deviceId = SDL_OpenAudioDevice(NULL, 0, audio_spec, NULL, 0);
		this->channels[i].sfx = -1;
		this->channels[i].offset = 0;
	}

	int size = 50000;

	std::vector<float> dest(size);
	memset(&dest[0], 0.0, size * sizeof(float));

	audio_generate_wave(audio_sin_wave, 220, dest, 0, 24860 + 110);
	audio_generate_wave(audio_sin_wave, 220, dest, 24860 + 110 - CROSS_FADE, size);

	SDL_QueueAudio(this->channels[0].deviceId, &dest[0], size * sizeof(float));
	SDL_PauseAudioDevice(this->channels[0].deviceId, 0);
}
AudioManager::~AudioManager()
{
	for (int i = 0; i < SFX_AMOUNT; i++) {
		if (this->cache[i] != NULL) {
			delete this->cache[i];
		}
	}

	for (int i = 0; i < CHANNELS; i++) {
		SDL_CloseAudioDevice(this->channels[i].deviceId);
		delete this->channels[i].spec;
	}
}

void AudioManager::initialize() {
	p8_memory[ADDR_HW_AUDIO_SAMPLE_PITCH] = 0;
	p8_memory[ADDR_HW_AUDIO_REBERB] = 0;
	p8_memory[ADDR_HW_AUDIO_BITCRUSH] = 0;
	p8_memory[ADDR_HW_AUDIO_DAMPEN] = 0;

	for (int i = 0; i < CHANNELS; i++) {
		SDL_PauseAudioDevice(this->channels[i].deviceId, 1);
		this->channels[i].sfx = -1;
		this->channels[i].offset = 0;
	}
	for (int i = 0; i < SFX_AMOUNT; i++) {
		if (this->cache[i] != NULL) {
			delete this->cache[i];
			this->cache[i] = NULL;
		}
	}
}

#define SFX_BYTE_LENGTH 68
int get_sfx_speed(int n) {
	int sfx_offset = ADDR_SFX + n * SFX_BYTE_LENGTH;
	int speed = p8_memory[sfx_offset + 64 + 1];
	if (speed < 1) {
		speed = 1;
	}
	return speed;
}

// Loops, n=-2 (manage from lua), channel=-2 (manage from lua)
std::vector<int16_t>* audio_buffer_from_sfx(int n);
void AudioManager::playSfx(int n, int channel, int offset, int length)
{
	for (int i = 0; i < CHANNELS; i++) {
		SDL_LockAudioDevice(this->channels[i].deviceId);
	}

	if (channel == -1) {
		// Option 1. Pick the first that's free or is playing the same sfx.
		for (int c = 0; channel == -1 && c < CHANNELS; c++) {
			Channel ch = this->channels[c];
			if (ch.sfx == -1 || ch.sfx == n) {
				channel = c;
			}
		}

		// Option 2. Kick the first channel to play yours
		if (channel == -1) {
			channel = 0;
		}
	}
	this->channels[channel].sfx = n;
	int speed = get_sfx_speed(n);
	this->channels[channel].offset = offset * speed * P8_TICKS_PER_T;
	this->channels[channel].max = (offset+length) * speed * P8_TICKS_PER_T;

	for (int i = 0; i < CHANNELS; i++) {
		SDL_UnlockAudioDevice(this->channels[i].deviceId);
	}
	SDL_PauseAudioDevice(this->channels[channel].deviceId, 0);
}

void AudioManager::stopSfx(int n)
{
	for (int i = 0; i < CHANNELS; i++) {
		SDL_LockAudioDevice(this->channels[i].deviceId);
	}

	for (int i = 0; i < CHANNELS; i++) {
		if (this->channels[i].sfx == n) {
			this->channels[i].sfx = -1;
			this->channels[i].offset = 0;
		}
	}

	for (int i = 0; i < CHANNELS; i++) {
		SDL_UnlockAudioDevice(this->channels[i].deviceId);
	}
}

void AudioManager::stopChannel(int channel)
{
	for (int i = 0; i < CHANNELS; i++) {
		SDL_LockAudioDevice(this->channels[i].deviceId);
	}

	this->channels[channel].sfx = -1;
	this->channels[channel].offset = 0;

	for (int i = 0; i < CHANNELS; i++) {
		SDL_UnlockAudioDevice(this->channels[i].deviceId);
	}
}

void audio_cb(void* userdata, Uint8* stream, int len) {
	Channel* channel = (Channel*)userdata;

	if (channel->sfx == -1) {
		memset(stream, 0, len);
		SDL_PauseAudioDevice(channel->deviceId, 1);
	}
	else {
		if (audioManager->cache[channel->sfx] == NULL) {
			audioManager->cache[channel->sfx] = audio_buffer_from_sfx(channel->sfx);
		}

		std::vector<int16_t>* cached = audioManager->cache[channel->sfx];
		int top = std::min(cached->size(), (size_t)channel->max);
		if (channel->offset*2 + len > top*2) {
			int copied_lenght = (top - channel->offset) * 2;
			memcpy(stream, &(*cached)[channel->offset], copied_lenght);
			memset(stream + copied_lenght, 0, len - copied_lenght);
			channel->sfx = -1;
			channel->offset = 0;
		}
		else {
			memcpy(stream, &(*cached)[channel->offset], len);
			channel->offset += len / 2;
		}
	}
}

void AudioManager::poke(unsigned short addr, unsigned char value)
{
	if (addr >= ADDR_SFX && addr < ADDR_SFX + SFX_BYTE_LENGTH * SFX_AMOUNT) {
		// Invalidate the buffer of that sfx
		// I don't think it's important if it's being played: It's not a buffer, but a cache. The actual buffer is on the hardware.
		int s = (addr - ADDR_SFX) / SFX_BYTE_LENGTH;
		for (int i = 0; i < CHANNELS; i++) {
			SDL_LockAudioDevice(this->channels[i].deviceId);
		}
		if (this->cache[s] != NULL) {
			delete this->cache[s];
		}
		for (int i = 0; i < CHANNELS; i++) {
			SDL_UnlockAudioDevice(this->channels[i].deviceId);
		}
	}
}

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

std::vector<int16_t>* audio_buffer_from_sfx(int s) {
	// Parse sfx
	P8_SFX sfx;
	int sfx_offset = ADDR_SFX + s * SFX_BYTE_LENGTH;
	for (int n = 0; n < NOTE_AMOUNT; n++) {
		int note_offset = sfx_offset + n * 2;
		sfx.notes[n].pitch = p8_memory[note_offset] & 0x3F;
		sfx.notes[n].instrument =
			p8_memory[note_offset] >> 6 |
			((p8_memory[note_offset + 1] & 0x01) << 2) |
			((p8_memory[note_offset + 1] & 0x80) >> 4);
		sfx.notes[n].volume = (p8_memory[note_offset + 1] >> 1) & 0x07;
		sfx.notes[n].effect = (p8_memory[note_offset + 1] >> 4) & 0x07;
	}
	unsigned char flags = p8_memory[sfx_offset + 64];
	sfx.noiz = (flags & 0x2) > 0;
	sfx.buzz = (flags & 0x4) > 0;
	sfx.detune = flags / 8 % 3;
	sfx.reverb = flags / 24 % 3;
	sfx.dampen = flags / 72 % 3;
	sfx.speed = get_sfx_speed(s);
	sfx.loopStart = p8_memory[sfx_offset + 64 + 2];
	sfx.loopEnd = p8_memory[sfx_offset + 64 + 3];

	// Build sfx buffer
	int num_notes = NOTE_AMOUNT;
	while (num_notes > 0 && sfx.notes[num_notes - 1].volume == 0) {
		num_notes--;
	}

	int total_ticks = sfx.speed * P8_TICKS_PER_T * num_notes;
	std::vector<float> floats(total_ticks);
	for (int i = 0; i < num_notes; i++) {
		if (sfx.notes[i].volume > 0) {
			audio_generate_note(sfx.notes[i], floats, sfx.speed * P8_TICKS_PER_T * i, sfx.speed * P8_TICKS_PER_T * (i + 1));
		}
	}

	std::vector<int16_t>* result = new std::vector<int16_t>(total_ticks);
	for (int n = 0; n < num_notes; n++) {
		if (sfx.notes[n].volume > 0) {
			for (int i = 0; i < sfx.speed * P8_TICKS_PER_T; i++) {
				(*result)[n * sfx.speed * P8_TICKS_PER_T + i] = sfx.notes[n].volume * floats[n * sfx.speed * P8_TICKS_PER_T + i] * 5000;
			}
		}
	}

	return result;
}

/// Wave generator

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
		return (1 + 2 * TILTED_TIP / (1 - TILTED_TIP)) - (phase / (1 - TILTED_TIP)) * 2;
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

void audio_generate_wave(float (*wave_fn)(float), unsigned int wavelength, std::vector<float>& dest, unsigned int from, unsigned int to) {
	// Generate the reference wave
	for (unsigned int i = 0; i < wavelength; i++) {
		dest[from + CROSS_FADE + i] = wave_fn((float)i / wavelength);
	}

	// Generate the fade in
	int fadein_offset = wavelength - (CROSS_FADE % wavelength);
	for (unsigned int i = 0; i < CROSS_FADE; i++) {
		dest[from + i] += dest[from + CROSS_FADE + ((i+fadein_offset) % wavelength)] * i / CROSS_FADE;
	}

	// Copy it until we reach the end
	unsigned int len = to - from;
	// memmove(&dest[from + CROSS_FADE], &dest[from + CROSS_FADE + wavelength], sizeof(float) * (len - 2 * CROSS_FADE));
	for (unsigned int i = CROSS_FADE + wavelength; i < len - CROSS_FADE; i++) {
		dest[from + i] = dest[from + CROSS_FADE + ((i - CROSS_FADE) % wavelength)];
	}

	// Fade out
	int fadeout_offset = (len - 2*CROSS_FADE) % wavelength;
	for (unsigned int i = 0; i < CROSS_FADE; i++) {
		dest[to - CROSS_FADE + i] += dest[from + CROSS_FADE + ((i + fadeout_offset) % wavelength)] * (CROSS_FADE - i - 1) / CROSS_FADE;
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
		dest[i + from] = volume * src[i];
	}
}
