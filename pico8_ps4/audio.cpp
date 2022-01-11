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
		ff = ff * 1.0293; // A quater step
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

		phase_shift = find_phaseshift(waveGenerator, prevSample * 7 / v, sample * 7 / v);
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
			dest[i] += prev_sample * v / 7;
		}
		else {
			float wavelength = audio_get_wavelength(current_freq);
			phase = std::fmod(phase + 1 / wavelength, 1); // TODO 1/wavelength = current_freq / SAMPLE_RATE
			dest[i] += waveGenerator(phase) * v / 7;
		}
	}
}

void audio_cb(void* userdata, Uint8* stream, int len);

AudioManager::AudioManager()
{
	for (int i = 0; i < CHANNELS; i++) {
		this->channels[i].sfx = -1;
		this->channels[i].offset = 0;
		this->channels[i].max = 0;
		this->channels[i].previousSample = 0;
		this->channels[i].previousSample2 = 0;
		this->channels[i].music_timing = -1;
		this->channels[i].reserved = false;
		this->channels[i].isMusic = false;
	}

	SDL_AudioSpec* audio_spec = new SDL_AudioSpec;
	// SDL_zero(audio_spec);
	audio_spec->freq = P8_SAMPLE_RATE;
	audio_spec->format = AUDIO_F32LSB;
	audio_spec->channels = 1; // mono/stereo/quad/5.1
	audio_spec->samples = 1024;
	audio_spec->callback = audio_cb;
	audio_spec->userdata = &this->channels;

	this->spec = audio_spec;
	this->deviceId = SDL_OpenAudioDevice(NULL, 0, audio_spec, NULL, 0);

	this->music_thread = std::thread(&AudioManager::music_loop, this);
	this->pattern = -1;
}
AudioManager::~AudioManager()
{
	SDL_CloseAudioDevice(this->deviceId);
	delete this->spec;
}

void AudioManager::initialize() {
	p8_memory[ADDR_HW_AUDIO_SAMPLE_PITCH] = 0;
	p8_memory[ADDR_HW_AUDIO_REBERB] = 0;
	p8_memory[ADDR_HW_AUDIO_BITCRUSH] = 0;
	p8_memory[ADDR_HW_AUDIO_DAMPEN] = 0;

	for (int i = 0; i < CHANNELS; i++) {
		this->channels[i].sfx = -1;
		this->channels[i].offset = 0;
		this->channels[i].max = 0;
		this->channels[i].previousSample = 0;
		this->channels[i].previousSample2 = 0;
		this->channels[i].music_timing = -1;
		this->channels[i].reserved = false;
		this->channels[i].isMusic = false;
	}
	SDL_PauseAudioDevice(this->deviceId, 1);
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

// N=-2 (manage from lua), channel=-2 (manage from lua)
void AudioManager::playSfx(int n, int channel, int offset, int length)
{
	SDL_LockAudioDevice(this->deviceId);

	if (channel == -1) {
		// Option 1. Out from the ones that are not reserved, pick the first that's free or is playing the same sfx.
		for (int c = 0; channel == -1 && c < CHANNELS; c++) {
			Channel ch = this->channels[c];
			if (ch.reserved) {
				continue;
			}
			if (ch.sfx == -1 || ch.sfx == n) {
				channel = c;
			}
		}

		// Option 2. From the reserved ones, use it if it's not playing anything at the moment (music will override it if needed)
		for (int c = 0; channel == -1 && c < CHANNELS; c++) {
			Channel ch = this->channels[c];
			if (ch.reserved && ch.sfx == -1) {
				channel = c;
			}
		}

		// Option 3. Kick the first non-reserved channel to play yours
		for (int c = 0; channel == -1 && c < CHANNELS; c++) {
			Channel ch = this->channels[c];
			if (!ch.reserved) {
				channel = c;
			}
		}

		if (channel == -1) {
			DEBUGLOG << "No channel available to play sfx. Skipping" << ENDL;
			return;
		}
	}
	this->channels[channel].sfx = n;
	int speed = get_sfx_speed(n);
	this->channels[channel].offset = offset * speed * P8_TICKS_PER_T;
	this->channels[channel].max = (offset+length) * speed * P8_TICKS_PER_T;
	this->channels[channel].previousSample = 0;
	this->channels[channel].previousSample2 = 0;

	SDL_UnlockAudioDevice(this->deviceId);
	SDL_PauseAudioDevice(this->deviceId, 0);
}

void AudioManager::playMusic(int n, unsigned char channelmask)
{
	SDL_LockAudioDevice(this->deviceId);

	for (int i = 0; i < CHANNELS; i++) {
		int bit = 0x01 << i;
		this->channels[i].reserved = (channelmask & bit) > 0;
	}
	this->playPattern(n);

	SDL_UnlockAudioDevice(this->deviceId);
}

void AudioManager::stopMusic()
{
	SDL_LockAudioDevice(this->deviceId);

	this->stopPattern();

	SDL_UnlockAudioDevice(this->deviceId);
}

void AudioManager::pause()
{
	SDL_PauseAudioDevice(this->deviceId, 1);
}

void AudioManager::resume()
{
	SDL_PauseAudioDevice(this->deviceId, 0);
}

void AudioManager::playNextPattern()
{
	int addr = ADDR_MUSIC + this->pattern * 4;
	bool endLoop = p8_memory[addr + 1] & 0x80;
	bool stopAtEnd = p8_memory[addr + 2] & 0x80;

	if (stopAtEnd) {
		return this->stopPattern();
	}
	if (endLoop) {
		// Find closest beginLoop
		// If it reaches 0 we just start from there
		int p = this->pattern;
		for (; p > 0; p--) {
			bool beginLoop = p8_memory[ADDR_MUSIC + p * 4] & 0x80;
			if (beginLoop) {
				break;
			}
		}
		return this->playPattern(p);
	}

	if (this->pattern == 63) {
		return this->stopPattern();
	}

	// Try next one
	int next_addr = addr + 4;
	bool has_data = (
		(p8_memory[addr] & 0x40) |
		(p8_memory[addr + 1] & 0x40) |
		(p8_memory[addr + 2] & 0x40) |
		(p8_memory[addr + 3] & 0x40)
		) > 0;
	if (has_data) {
		this->playPattern(this->pattern + 1);
	}
	else {
		this->stopPattern();
	}
}

typedef struct {
	int n;
	int speed;
	int len;
	bool loops;
} PatternSfx;

void AudioManager::playPattern(int n)
{
	int addr = ADDR_MUSIC + n * 4;

	std::vector<PatternSfx> sfxs;
	for (int i = 0; i < 4; i++) {
		unsigned char data = p8_memory[addr + i];
		bool enabled = (data & 0x40) == 0;

		if (enabled) {
			int n = data & 0x3F;
			int speed = get_sfx_speed(n);
			unsigned char loopStart = p8_memory[ADDR_SFX + n * SFX_BYTE_LENGTH + 64 + 2];
			unsigned char loopEnd = p8_memory[ADDR_SFX + n * SFX_BYTE_LENGTH + 64 + 3];
			bool loops = loopEnd > 0;
			int len = 32;
			if (loopEnd == 0 && loopStart > 0) {
				len = loopStart;
			}
			sfxs.push_back(PatternSfx{
				n,
				speed,
				len,
				loops
			});
		}
	}
	if (sfxs.size() == 0) {
		DEBUGLOG << "Pattern disabled, can't play it: " << n << ENDL;
		return;
	}

	bool same_tempo = true;
	bool all_loop = sfxs[0].loops;
	int i_slowest = 0;
	int slowest_speed = sfxs[0].speed;
	int i_first_no_loop = -1;
	if (!sfxs[0].loops) {
		i_first_no_loop = 0;
	}
	for (int i = 1; i < sfxs.size(); i++) {
		same_tempo = same_tempo && (sfxs[0].speed == sfxs[i].speed);
		all_loop = all_loop && sfxs[i].loops;
		if (sfxs[i].speed > slowest_speed) { // Note that a higher speed value means more slow
			slowest_speed = sfxs[i].speed;
			i_slowest = i;
		}
		if (i_first_no_loop == -1 && !sfxs[i].loops) {
			i_first_no_loop = i;
		}
	}

	// All same tempo => first
	// else, All loop => slowest
	// else, First that doesn't loop
	int i_music_timing;
	if (same_tempo) {
		i_music_timing = 0;
	}
	else if (all_loop) {
		i_music_timing = i_slowest;
	}
	else {
		i_music_timing = i_first_no_loop;
	}

	this->playPatternSfx(sfxs[i_music_timing].n, sfxs[i_music_timing].len);
	for (int i = 0; i < sfxs.size(); i++) {
		if (i == i_music_timing) continue;

		this->playPatternSfx(sfxs[i].n, -1);
	}

	this->pattern = n;
}

void AudioManager::playPatternSfx(int n, int timing_length)
{
	int channel = -1;

	// Option 1. Grab a free one from the reserved ones.
	for (int c = 0; channel == -1 && c < CHANNELS; c++) {
		Channel ch = this->channels[c];
		if (!ch.reserved) {
			continue;
		}
		if (ch.sfx == -1) {
			channel = c;
		}
	}

	// Option 2. Grab anyone that's free
	for (int c = 0; channel == -1 && c < CHANNELS; c++) {
		Channel ch = this->channels[c];
		if (ch.sfx == -1) {
			channel = c;
		}
	}

	if (channel == -1) {
		DEBUGLOG << "Pattern skipped, no channels available" << n << ENDL;
		return;
	}

	this->channels[channel].sfx = n;
	this->channels[channel].offset = 0;
	this->channels[channel].isMusic = true;
	this->channels[channel].music_timing = timing_length;
	SDL_PauseAudioDevice(this->deviceId, 0);
}

void AudioManager::stopPattern()
{
	for (int i = 0; i < CHANNELS; i++) {
		// Free all channels
		this->channels[i].reserved = false;
		this->channels[i].music_timing = -1;

		// Stop music channels
		if (this->channels[i].isMusic) {
			this->channels[i].isMusic = false;
			this->channels[i].offset = 0;
			this->channels[i].sfx = -1;
		}
	}
	this->pattern = -1;
}

void AudioManager::stopSfx(int n)
{
	SDL_LockAudioDevice(this->deviceId);

	for (int i = 0; i < CHANNELS; i++) {
		if (this->channels[i].sfx == n) {
			this->channels[i].sfx = -1;
			this->channels[i].offset = 0;
		}
	}

	SDL_UnlockAudioDevice(this->deviceId);
}

void AudioManager::stopChannel(int channel)
{
	SDL_LockAudioDevice(this->deviceId);

	this->channels[channel].sfx = -1;
	this->channels[channel].offset = 0;

	SDL_UnlockAudioDevice(this->deviceId);
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
	// Does nothing
}

void AudioManager::music_loop()
{
	bool dummy;
	while (this->music_notifier.wpop(dummy)) {
		SDL_LockAudioDevice(this->deviceId);

		for (int i = 0; i < CHANNELS; i++) {
			// Stop all music looping channels, otherwise they will be marked as not available.
			if (this->channels[i].isMusic) {
				this->channels[i].sfx = -1;
				this->channels[i].offset = 0;
			}
		}
		this->playNextPattern();

		SDL_UnlockAudioDevice(this->deviceId);
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

int audio_cb_channel(Channel* channel, float* stream, int data_points) {
	if (channel->sfx == -1) {
		return 0;
	}

	P8_SFX sfx = get_sfx(channel->sfx);
	int current_index = channel->offset / (sfx.speed * P8_TICKS_PER_T);
		
	if (current_index >= NOTE_AMOUNT) {
		// We're just waiting for loop to come back (it can be grater than NOTE_AMOUNT)
		if (sfx.loopEnd < current_index) {
			DEBUGLOG << "SFX out of range. This should not happen" << ENDL;
			channel->sfx = -1;
			channel->offset = 0;
			return 0;
		}

		int ticks_until_loop = sfx.loopEnd * sfx.speed * P8_TICKS_PER_T - channel->offset;
		if (ticks_until_loop < data_points) {
			channel->offset = sfx.loopStart * sfx.speed * P8_TICKS_PER_T;
			return -audio_cb_channel(channel, &stream[ticks_until_loop], data_points - ticks_until_loop);
		}
		else {
			channel->offset += data_points;
		}
		return 0;
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
	if (endOffset == 0) {
		endOffset = 1;
	}

	float prevValue2 = stream[length - 2];
	float prevValue = stream[length - 1];
	generate_next_samples(
		stream, length,
		channel->previousSample2, channel->previousSample,
		freq, currentNote.instrument, currentNote.volume, currentNote.effect,
		prevFreq, offset, endOffset
	);
	channel->previousSample2 = stream[length - 2] - prevValue2;
	channel->previousSample = stream[length - 1] - prevValue;

	if (channel->offset == end_of_note) {
		int next_note = current_index + 1;
		if (channel->music_timing > 0) {
			channel->music_timing--;
			if (channel->music_timing == 0) {
				audioManager->music_notifier.push(true);
			}
		}

		// Look for loop so we can start putting data from the next loop beginning.
		if (sfx.loopEnd == next_note) {
			channel->offset = sfx.loopStart * sfx.speed * P8_TICKS_PER_T;
			int t = audio_cb_channel(channel, &stream[length], data_points - length);
			return length + t;
		}

		// If we don't have more notes to play, finish here
		bool has_more = false;
		for (int i = current_index + 1; i < NOTE_AMOUNT && !has_more; i++) {
			has_more = sfx.notes[i].volume > 0;
		}

		if (!has_more && sfx.loopEnd < NOTE_AMOUNT) {
			if (channel->music_timing < 0) {
				channel->sfx = -1;
				channel->offset = 0;
			}
		}
		else if(length < data_points) {
			// Try to fill in as much as posible from the next note
			int t = audio_cb_channel(channel, &stream[length], data_points - length);
			return length + t;
		}
	}
	return length;
}

int get_overlaps(int (&lengths)[CHANNELS], int len, int offset) {
	int t = 0;
	for (int i = 0; i < CHANNELS; i++) {
		if (lengths[i] >= 0 && offset < lengths[i]) {
			t++;
		}
		if (lengths[i] < 0 && offset >= len + lengths[i]) {
			t++;
		}
	}
	if (t == 0) {
		return 1;
	}
	return t;
}
void audio_cb(void* userdata, Uint8* stream, int len) {
	Channel (*channels)[CHANNELS] = (Channel(*)[CHANNELS])userdata;
	int data_points = len / sizeof(float);
	float* buffer = (float*)stream;

	memset(stream, 0, len);
	int lengths[CHANNELS] = { 0,0,0,0 };

	for (int i = 0; i < CHANNELS; i++) {
		lengths[i] = audio_cb_channel(&(*channels)[i], buffer, data_points);
	}

	for (int i = 0; i < data_points; i++) {
		buffer[i] /= get_overlaps(lengths, data_points, i);
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
