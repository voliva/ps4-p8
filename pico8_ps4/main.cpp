#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <thread>

#include "log.h"
#include "cartridge.h"
#include "renderer.h"
#include "events.h"
#include "lua_state.h"
#include "machine_state.h"
#include <math.h>

#ifdef __PS4__
#define MINEWALKER "/app0/assets/misc/minewalker.p8.png"
#else
#define MINEWALKER "../assets/misc/minewalker.p8.png"
#endif

int millisecs_per_frame(bool is60Fps);

#define DEBUGLOG Main_DEBUGLOG
Log DEBUGLOG = logger.log("main");
MachineState* machineState;
int main(void)
{
	DEBUGLOG << "Initialize" << ENDL;
	if (!init_renderer()) {
		return -1;
	}

	DEBUGLOG << "Initialized" << ENDL;

	// the representation of our audio device in SDL:
	SDL_AudioDeviceID audio_device;

	// opening an audio device:
	SDL_AudioSpec audio_spec;
	SDL_zero(audio_spec);
	audio_spec.freq = 44100;
	audio_spec.format = AUDIO_S16SYS;
	audio_spec.channels = 1;
	audio_spec.samples = 1024;
	audio_spec.callback = NULL;

	DEBUGLOG << "Open audio device" << ENDL;
	audio_device = SDL_OpenAudioDevice(
		NULL, 0, &audio_spec, NULL, 0);
	DEBUGLOG << "Opened? " << audio_device << ENDL;

	// pushing 3 seconds of samples to the audio buffer:
	float x = 0;
	float hz = 440;
	float inc = hz * 2 * M_PI / audio_spec.freq;
	int AMP = 5000;
	for (int i = 0; i < audio_spec.freq * 1; i++) {
		x += inc;

		// SDL_QueueAudio expects a signed 16-bit value
		// note: "5000" here is just gain so that we will hear something
		// SIN
		int16_t sample = sin(x) * AMP * 1.5;

		// Square
		/*int16_t sample = 0;
		if ((int)(x / M_PI) % 2) {
			sample = AMP;
		}else{
			sample = -AMP;
		}*/

		// Triangle
		/*float mod = fmod(x, 2 * M_PI);
		int16_t sample = (abs(mod / M_PI - 1) - 0.5) * 2 * AMP * 2;*/

		// Sawtooth
		/*float mod = fmod(x, 2 * M_PI);
		int16_t sample = (mod / M_PI - 1) * AMP * 2;*/

		// White noise
		// int16_t sample = AMP * rand() / RAND_MAX;

		const int sample_size = sizeof(int16_t) * 1;
		SDL_QueueAudio(audio_device, &sample, sample_size);
	}

	DEBUGLOG << "Queued, playing" << ENDL;
	// unpausing the audio device (starts playing):
	SDL_PauseAudioDevice(audio_device, 0);

	DEBUGLOG << "Wait" << ENDL;
	SDL_Delay(1000);
	return -1;

	DEBUGLOG << "Close" << ENDL;
	SDL_CloseAudioDevice(audio_device);

	DEBUGLOG << "Loading cartridge..." << ENDL;
	Cartridge* r = load_from_png(MINEWALKER);

	load_spritesheet(r->sprite_map);

	LuaState luaState;
	if (!luaState.loadProgram(r->lua)) {
		DEBUGLOG << "Failed loading lua code from cartridge" << ENDL;
		return -1;
	}
	machineState = new MachineState();
	luaState.run_init();

	// Initialize input / joystick
	if (SDL_NumJoysticks() > 0)
	{
		SDL_JoystickOpen(0);
	}

	// Enter the render loop
	DEBUGLOG << "Entering render loop..." << ENDL;

	SDL_Event e;
	bool quit = false;
	unsigned char time_debt = 0;
	short ms_per_frame = millisecs_per_frame(luaState.is60FPS);
	while (!quit) {
		machineState->registerFrame();

		// When dragging the window the app pauses, on that case, ignore frame_start
		// https://stackoverflow.com/questions/29552658/how-do-you-fix-a-program-from-freezing-when-you-move-the-window-in-sdl2
		// auto frame_start = std::chrono::high_resolution_clock::now();
		while (SDL_PollEvent(&e) != 0)
		{
			// DEBUGLOG << e.type << ENDL;

			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
				quit = true;
			}

			KeyEvent* result = mapSdlEvent(e);

			if (result != NULL) {
				machineState->processKeyEvent(*result);
			}

			delete result;
		}
		auto frame_start = std::chrono::high_resolution_clock::now();

		luaState.run_update();

		if (time_debt < ms_per_frame / 2) {
			luaState.run_draw();
			clip_outside();
			SDL_UpdateWindowSurface(window);
		}
		else {
			DEBUGLOG << "Skipped frame. time debt = " << time_debt << ENDL;
		}

		auto frame_end = std::chrono::high_resolution_clock::now();
		auto timediff = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start).count();
		int remainingTime = ms_per_frame - timediff;
		if (remainingTime > 0) {
			if (remainingTime > time_debt) {
				time_debt = 0;
			}
			else {
				time_debt -= remainingTime;
			}
		}
		else {
			if ((int)time_debt + -remainingTime >= 255) {
				time_debt = 255;
			}
			else {
				time_debt += -remainingTime;
			}
		}

		if (remainingTime > 1) {
			// DEBUGLOG << "sleep for " << remainingTime << ENDL;
			std::this_thread::sleep_for(std::chrono::milliseconds(remainingTime));
		}
	}

	SDL_Quit();

	return 0;
}

int millisecs_per_frame(bool is60Fps) {
	int fps = 30;
	if (is60Fps) {
		fps = 60;
	}

	return 1000 / fps;
}