#include "running-cart.h"
#include "log.h"
#include "memory.h"
#include "audio.h"
#include "machine_state.h"
#include "font.h"
#include "lua_state.h"
#include "chrono.h"
#include <thread>
#include "pause_menu.h"

#define DEBUGLOG Runtime_DEBUGLOG
Log DEBUGLOG = logger.log("Runtime");

LuaState* luaState = NULL;
bool RunningCart::load(Cartridge* cartridge)
{
    if (this->status != RunningStatus::None && this->status != RunningStatus::Loaded) {
        DEBUGLOG << "Can't load new cartridge: Already running another one." << ENDL;
        return false;
    }

    DEBUGLOG << "Initializing memory" << ENDL;
    memory_load_cartridge(*cartridge);

    audioManager->initialize();
    renderer->initialize();
    machineState->initialize();
    font->initialize();

	if (luaState != NULL) {
		delete luaState;
	}
    luaState = new LuaState();

	DEBUGLOG << "Loading LUA" << ENDL;
	if (!luaState->loadProgram(cartridge->lua)) {
		DEBUGLOG << "Failed loading lua code from cartridge" << ENDL;
		return false;
	}

    DEBUGLOG << "Cartridge succesfully loaded" << ENDL;
    this->loadedCartridge = cartridge;
	this->paused = false;
    this->status = RunningStatus::Loaded;

    return true;
}

void RunningCart::run()
{
    if (this->status != RunningStatus::Loaded) {
        DEBUGLOG << "Can't run cartridge: Already running another one or not loaded." << ENDL;
        return;
    }

	this->runOnce();

	while (this->status == RunningStatus::Restarting) {
		DEBUGLOG << "Restart: Reinitializing memory" << ENDL;
		memory_load_cartridge(*this->loadedCartridge);

		audioManager->initialize();
		renderer->initialize();
		machineState->initialize();
		font->initialize();

		if (luaState != NULL) {
			delete luaState;
		}
		luaState = new LuaState();

		DEBUGLOG << "Loading LUA" << ENDL;
		luaState->loadProgram(this->loadedCartridge->lua);

		this->paused = false;

		// And run again
		this->runOnce();
	}

	this->status = RunningStatus::Loaded;
}

void RunningCart::stop()
{
    this->status = RunningStatus::Stopping;
	audioManager->pause();
}

void RunningCart::restart()
{
    this->status = RunningStatus::Restarting;
}

void RunningCart::pause()
{
	this->paused = true;
	audioManager->pause();
}

void RunningCart::resume()
{
	this->paused = false;
	audioManager->resume();
}

int millisecs_per_frame(bool is60Fps) {
	int fps = 30;
	if (is60Fps) {
		fps = 60;
	}

	return 1000 / fps;
}

void RunningCart::runOnce()
{
	DEBUGLOG << "Run _init" << ENDL;
	luaState->run_init();

	// Enter the render loop
	DEBUGLOG << "Entering render loop..." << ENDL;

	SDL_Event e;

	unsigned char time_debt = 0;
	short ms_per_frame = millisecs_per_frame(luaState->is60FPS);

	this->status = RunningStatus::Running;
	int frame = 0;
	while (this->status == RunningStatus::Running) {
		frame++;
		machineState->registerFrame();

		// When dragging the window the app pauses, on that case, ignore frame_start
		// https://stackoverflow.com/questions/29552658/how-do-you-fix-a-program-from-freezing-when-you-move-the-window-in-sdl2
		// auto frame_start = std::chrono::high_resolution_clock::now();
		while (SDL_PollEvent(&e) != 0)
		{
			// DEBUGLOG << e.type << ENDL;
			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
				this->stop();
			}

			KeyEvent* result = mapSdlEvent(e);

			if (result != NULL) {
				if (this->paused) {
					pauseMenu->manageEvent(*result);
				}
				else if (result->down && result->key == P8_Key::pause) {
					this->pause();
				}
				else {
					machineState->processKeyEvent(*result);
				}
			}

			delete result;
		}
		auto frame_start = getTimestamp();

		if (this->status == RunningStatus::Running) {
			if (this->paused) {
				pauseMenu->draw();
			}
			else {
				luaState->run_update();

				if (time_debt < ms_per_frame / 2) {
					luaState->run_draw();
					renderer->present();
				}
				else {
					DEBUGLOG << "Skipped frame. time debt = " << time_debt << ENDL;
				}
			}
		}

		auto timediff = getMillisecondsDiff(getTimestamp(), frame_start);
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
}
