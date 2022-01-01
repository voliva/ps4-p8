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
#include "audio.h"

#ifdef __PS4__
#define MINEWALKER "/app0/assets/misc/minewalker.p8.png"
#else
#define MINEWALKER "../assets/misc/minewalker.p8.png"
#endif

int millisecs_per_frame(bool is60Fps);

#define DEBUGLOG Main_DEBUGLOG
Log DEBUGLOG = logger.log("main");
MachineState* machineState;
AudioManager* audioManager;
int main(void)
{
	if (!init_renderer()) {
		return -1;
	}

	DEBUGLOG << "Initializing audio..." << ENDL;
	audioManager = new AudioManager();

	DEBUGLOG << "Loading cartridge..." << ENDL;
	Cartridge* r = load_from_png(MINEWALKER);

	DEBUGLOG << "Parsing spritesheet" << ENDL;
	load_spritesheet(r->sprite_map);

	DEBUGLOG << "Loading SFX" << ENDL;
	audioManager->loadSfx(r->sfx);

	LuaState luaState;

	DEBUGLOG << "Loading LUA" << ENDL;
	if (!luaState.loadProgram(r->lua)) {
		DEBUGLOG << "Failed loading lua code from cartridge" << ENDL;
		return -1;
	}
	machineState = new MachineState();

	// Initialize input / joystick
	if (SDL_NumJoysticks() > 0)
	{
		DEBUGLOG << "Initialize joysticks" << ENDL;
		SDL_JoystickOpen(0);
	}

	/* Not needed, and doesn't work on PS4
	DEBUGLOG << "Wait for screen ready..." << ENDL;
	if (!renderer_wait_until_ready()) {
		return 0;
	}
	*/

	DEBUGLOG << "Run _init" << ENDL;
	luaState.run_init();

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