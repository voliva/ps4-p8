#include "runtime.h"
#include "log.h"
#include "memory.h"
#include "audio.h"
#include "machine_state.h"
#include "font.h"
#include "lua_state.h"

#include <thread>

#define DEBUGLOG Runtime_DEBUGLOG
Log DEBUGLOG = logger.log("Runtime");

int millisecs_per_frame(bool is60Fps);

bool run_cartridge(Cartridge* r)
{
	memory_load_cartridge(*r);

	audioManager->initialize();
	renderer->initialize();
	machineState->initialize();
	font->initialize();

	LuaState luaState;

	DEBUGLOG << "Loading LUA" << ENDL;
	if (!luaState.loadProgram(r->lua)) {
		DEBUGLOG << "Failed loading lua code from cartridge" << ENDL;
		return false;
	}

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
			renderer->present();
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

	return true;
}

int millisecs_per_frame(bool is60Fps) {
	int fps = 30;
	if (is60Fps) {
		fps = 60;
	}

	return 1000 / fps;
}