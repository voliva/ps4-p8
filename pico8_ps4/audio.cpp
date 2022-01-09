#include "audio.h"
// #include <math.h>
#include <cmath>
#include "log.h"
#include "memory.h"

#define M_PI 3.14159265358979323

#define DEBUGLOG Audio_DEBUGLOG
Log DEBUGLOG = logger.log("audio");

int signIndex(float f) {
	if (f < 0) {
		return 0;
	}
	return 1;
}

// It will try and return a phaseshift that matches sample, with the same slope as (sample-prevSample)
#define ACCEPTABLE_STEP 0.01
float find_phaseshift(float (*wave_fn)(float), int sign, float sample, float from, float to) {
	float inc = (to - from) / 8;
	float bestDiffs[] = { 2.0, 2.0 };
	float bestFs[] = { -1.0, -1.0 };
	// We need to skip the first and the last: They have the same value that crosses the origin, and their range will be explored on the recursive call anyway.
	for (float f = from + inc; f < to; f += inc) {
		float v = wave_fn(f);
		float diff = std::fabs(sample - v);
		float nextV = wave_fn(std::nextafter(f, to));
		int sign = signIndex(nextV - v);

		if (bestDiffs[sign] > diff) {
			bestDiffs[sign] = diff;
			bestFs[sign] = f;
		}
	}

	if (inc < 0.0001) {
		if (bestFs[sign] >= 0) {
			return bestFs[sign];
		}
		return bestFs[1 - sign];
	}

	// First try same sign, then neutral, then opposite sign
	if (bestDiffs[sign] < ACCEPTABLE_STEP) {
		return bestFs[sign];
	}

	// Try to improve it
	if (bestFs[sign] != -1) {
		bestFs[sign] = find_phaseshift(wave_fn, sign, sample, std::fmax(bestFs[sign] - inc, 0), std::fmin(bestFs[sign] + inc, 1));
		bestDiffs[sign] = std::fabs(sample - wave_fn(bestFs[sign]));
		if (bestDiffs[sign] < ACCEPTABLE_STEP) {
			return bestFs[sign];
		}
	}

	// Same now for opposite sign
	if (bestDiffs[1 - sign] < ACCEPTABLE_STEP) {
		return bestFs[1 - sign];
	}
	if (bestFs[1-sign] != -1) {
		bestFs[1-sign] = find_phaseshift(wave_fn, sign, sample, std::fmax(bestFs[1-sign] - inc, 0), std::fmin(bestFs[1-sign] + inc, 1));
		bestDiffs[1-sign] = std::fabs(sample - wave_fn(bestFs[1-sign]));
		if (bestDiffs[1-sign] < ACCEPTABLE_STEP) {
			return bestFs[1-sign];
		}
	}

	// Otherwise just return the closest match
	if (bestDiffs[sign] < bestDiffs[1 - sign]) {
		return bestFs[sign];
	}
	return bestFs[1 - sign];
}
float find_phaseshift(float (*wave_fn)(float), float prevSample, float sample) {
	float diff = sample - prevSample;
	return find_phaseshift(wave_fn, signIndex(diff), sample, 0, 1);
}

typedef float(*WaveGenerator)(float);

// float *(float) instruments[] = {};
WaveGenerator instruments[] = { audio_triangle_wave, audio_tilted_wave, audio_sawtooth_wave, audio_square_wave, audio_pulse_wave, audio_organ_wave, audio_noise_wave, audio_phaser_wave };

float lerp(float from, float to, float offset) {
	return from + (to - from) * offset;
}

// How to do arpeggios? => Another function /shrug
void generate_next_samples(
	float* dest, int length,
	float prevSample, float sample,
	float freq, int instrument, int volume, int effect,
	float prevFreq, float offset, float endOffset
) {
	// instrument = 0;
	// volume = 5;
	// effect = 0;

	if (instrument >= 8) {
		// TODO custom instruments
		instrument = 0;
	}
	WaveGenerator waveGenerator = instruments[instrument];

	float f0 = freq;
	float ff = f0;
	bool vibrato = false;
	if (effect == 1) { // Slide
		f0 = prevFreq;
	} else if (effect == 2) { // Vibrato. Pitches love vibrato.
		// ff = ff * 1.059; // Bring up half step => Too much
		ff = ff * 1.03;
		vibrato = true;
	} else if (effect == 3) { // Drop
		ff = 0;
	}

	int v0 = volume;
	int vf = v0;
	if (effect == 4) { // Fade in
		v0 = 0;
	} else if (effect == 5) { // Fade out
		vf = 0;
	}

	float phase_shift = 0;
	if (instrument != 6) {
		float v = lerp(v0, vf, offset);

		// phase_shift = find_phaseshift(waveGenerator, prevSample, sample);
		phase_shift = find_phaseshift(waveGenerator, prevSample * 7 / v, sample * 7 / v);
		// DEBUGLOG << phase_shift << ": " << (waveGenerator(phase_shift) * v / 7) << " == " << sample << ENDL;
	}

	// Initial freq_offset
	float phase = phase_shift;
	float prev_sample = sample; // Needed for noise generator
	for (int i = 0; i < length; i++) {
		float inner_offset = lerp(offset, endOffset, (float)i / length);

		// Volume
		float v = lerp(v0, vf, inner_offset);

		// Frequency
		float freq_offset = inner_offset;
		if (vibrato) {
			freq_offset = std::sinf(4 * inner_offset * 2 * M_PI - M_PI / 4) / 2;
		}
		float current_freq = lerp(f0, ff, freq_offset);

		if (instrument == 6) {
			float max_diff = lerp(0, 1, current_freq / 5000);
			float scale = lerp(3, 0.4, current_freq / 8000); //  ~95 => 2 380 => 1.5
			float next = audio_noise_wave(0);
			float diff = std::fmax(std::fmin(next - prev_sample, max_diff), -max_diff);
			prev_sample = std::fmax(-1, std::fmin(1, prev_sample + diff * scale));
			dest[i] = prev_sample * v;
		}
		else {
			float wavelength = audio_get_wavelength(current_freq);
			phase = std::fmod(phase + 1 / wavelength, 1); // TODO 1/wavelength = current_freq / SAMPLE_RATE
			dest[i] = waveGenerator(phase) * v / 7;
		}
	}
}

void audio_cb(void* userdata, Uint8* stream, int len);

AudioManager::AudioManager()
{
	for (int i = 0; i < CHANNELS; i++) {
		SDL_AudioSpec* audio_spec = new SDL_AudioSpec;
		// SDL_zero(audio_spec);
		audio_spec->freq = P8_SAMPLE_RATE;
		audio_spec->format = AUDIO_F32LSB;
		audio_spec->channels = 1; // mono/stereo/quad/5.1
		audio_spec->samples = 1024;
		audio_spec->callback = audio_cb;
		audio_spec->userdata = &this->channels[i];

		this->channels[i].spec = audio_spec;
		this->channels[i].deviceId = SDL_OpenAudioDevice(NULL, 0, audio_spec, NULL, 0);
		this->channels[i].sfx = -1;
		this->channels[i].offset = 0;
	}

	int notes = 4;
	int note_length = P8_SAMPLE_RATE / 1;
	int size = notes * note_length;

	std::vector<float> dest(size);
	memset(&dest[0], 0.0, size * sizeof(float));
}
AudioManager::~AudioManager()
{
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
	this->channels[channel].previousSample = 0;
	this->channels[channel].previousSample2 = 0;

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

P8_SFX get_sfx(int s) {
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

	return sfx;
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


void audio_cb(void* userdata, Uint8* stream, int len) {
	Channel* channel = (Channel*)userdata;
	int data_points = len / sizeof(float);

	memset(stream, 0, len);
	if (channel->sfx == -1) {
		memset(stream, 0, len);
		SDL_PauseAudioDevice(channel->deviceId, 1);
	}
	else {
		P8_SFX sfx = get_sfx(channel->sfx);
		int current_index = channel->offset / (sfx.speed * P8_TICKS_PER_T);
		
		if (current_index >= NOTE_AMOUNT) {
			// We're just waiting for loop to come back (it can be grater than NOTE_AMOUNT)
			memset(stream, 0, len);
			if (sfx.loopEnd < current_index) {
				DEBUGLOG << "SFX out of range. This should not happen" << ENDL;
				channel->sfx = -1;
				channel->offset = 0;
				return;
			}

			int ticks_until_loop = sfx.loopEnd * sfx.speed * P8_TICKS_PER_T - channel->offset;
			if (ticks_until_loop < data_points) {
				channel->offset = sfx.loopStart * sfx.speed * P8_TICKS_PER_T;
				audio_cb(userdata, &stream[ticks_until_loop * sizeof(float)], len - ticks_until_loop * sizeof(float));
			}
			else {
				channel->offset += data_points;
			}
			return;
		}

		P8_Note currentNote = sfx.notes[current_index];
		float freq = pitch_to_freq(currentNote.pitch);

		float prevFreq = freq;
		if (current_index > 0) {
			// TODO Assumption will break when loops => Move to Channel?
			prevFreq = pitch_to_freq(sfx.notes[current_index - 1].pitch);
		}
		float offset = (float)(channel->offset % (sfx.speed * P8_TICKS_PER_T)) / (sfx.speed * P8_TICKS_PER_T);

		// Fill until end of note
		int end_of_note = (current_index + 1) * sfx.speed * P8_TICKS_PER_T;
		int length = std::min(data_points, end_of_note - (int)channel->offset);
		channel->offset += length;
 		float endOffset = (float)(channel->offset % (sfx.speed * P8_TICKS_PER_T)) / (sfx.speed * P8_TICKS_PER_T);

		generate_next_samples(
			(float*)stream, length,
			channel->previousSample2, channel->previousSample,
			freq, currentNote.instrument, currentNote.volume, currentNote.effect,
			prevFreq, offset, endOffset
		);
		channel->previousSample2 = ((float*)stream)[length - 2];
		channel->previousSample = ((float*)stream)[length - 1];

		if (channel->offset == end_of_note) {
			int next_note = current_index + 1;

			// Look for loop
			if (sfx.loopEnd == next_note) {
				channel->offset = sfx.loopStart * sfx.speed * P8_TICKS_PER_T;
				// Try to fill in as much as posible from the next note
				audio_cb(userdata, &stream[length * sizeof(float)], (data_points - length) * sizeof(float));
				return;
			}

			// If we don't have more notes to play, finish here
			bool has_more = false;
			for (int i = current_index + 1; i < NOTE_AMOUNT && !has_more; i++) {
				has_more = sfx.notes[i].volume > 0;
			}

			if (!has_more) {
				channel->sfx = -1;
				channel->offset = 0;
			}
			else if(length < data_points) {
				// Try to fill in as much as posible from the next note
				audio_cb(userdata, &stream[length * sizeof(float)], (data_points - length) * sizeof(float));
			}
		}
	}
}

/// Wave generator

unsigned int audio_get_wavelength(float frequency) {
	return P8_SAMPLE_RATE / std::fmax(frequency, 1);
}
unsigned int audio_get_points(float seconds) {
	return P8_SAMPLE_RATE * seconds;
}

float audio_sin_wave(float phase) {
	return sinf(phase * 2 * M_PI);
}
float audio_triangle_wave(float phase) {
	return (fabs(phase - 0.5) * -4 + 1) * 0.7;
}

#define TILTED_TIP 0.9
float audio_tilted_wave(float phase) {
	if (phase < TILTED_TIP) {
		return ((phase / TILTED_TIP) * 2 - 1) * 0.7;
	}
	else {
		return ((1 + 2 * TILTED_TIP / (1 - TILTED_TIP)) - (phase / (1 - TILTED_TIP)) * 2) * 0.7;
	}
}
float audio_sawtooth_wave(float phase) {
	return (phase * 2 - 1) * 0.45;
}
float audio_square_wave(float phase) {
	if (phase < 0.5) {
		return -1.0/3;
	}
	else {
		return 1.0/3;
	}
}
float audio_pulse_wave(float phase) {
	if (phase < 0.75) {
		return -1.0/3;
	}
	else {
		return 1.0/3;
	}
}
float audio_organ_wave(float phase) {
	return (audio_triangle_wave(phase) + audio_triangle_wave(fmod(phase * 2, 1))) / 2;
}
float audio_noise_wave(float phase) {
	return ((float)rand() / RAND_MAX * 2) - 1;
}
float audio_phaser_wave(float phase) {
	return (audio_triangle_wave(phase) + audio_triangle_wave(fmod(phase * 0.99, 1))) / 2;
}
