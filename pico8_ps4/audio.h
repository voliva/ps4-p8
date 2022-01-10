#pragma once

#include <vector>;
#include <SDL2/SDL_audio.h>
#include <thread>
#include "concurrent_queue.h"

#define P8_SAMPLE_RATE 22050
#define SFX_AMOUNT 64
#define CHANNELS 4

typedef struct {
	SDL_AudioDeviceID deviceId;
	SDL_AudioSpec* spec;
	int sfx; // -1 = paused
	unsigned int offset;
	unsigned int max;

	// stuff played earlier
	float previousSample;
	float previousSample2;

	// Whether this channel sets the point where the next pattern must start
	int music_timing;
	bool reserved;
	bool isMusic; // Is currently playing music
} Channel;

class AudioManager {
public:
	AudioManager();
	~AudioManager();
	void initialize();
	void playSfx(int n, int channel, int offset, int length);
	void stopSfx(int n);
	void playMusic(int n, unsigned char channelmask);
	void stopMusic();
	void pause();
	void resume();
	void stopChannel(int channel);
	void poke(unsigned short addr, unsigned char value);

	Channel channels[4];

	ConcurrentQueue<bool> music_notifier;

private:
	// We need a function outside the audio thread to lock all the channels and switch them to the next pattern
	std::thread music_thread;
	void music_loop();

	int pattern; // -1 = not active
	void playNextPattern();
	void playPattern(int n);
	void playPatternSfx(int n, int timing_length);
	void stopPattern();
};
extern AudioManager* audioManager;

typedef struct {
	unsigned char instrument; // [0-7 default instruments, 8-15 custom instruments]
	unsigned char volume; // [0-7]
	unsigned char effect; // [0-7]
	unsigned char pitch; // [0-63]
} P8_Note;

#define P8_TICKS_PER_T 183
#define NOTE_AMOUNT 32
typedef struct {
	bool noiz;
	bool buzz;
	char detune;
	char reverb;
	char dampen;
	char speed;
	char loopStart;
	char loopEnd;
	P8_Note notes[NOTE_AMOUNT];
} P8_SFX;

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
float audio_noise_wave(float phase);
float audio_phaser_wave(float phase);
