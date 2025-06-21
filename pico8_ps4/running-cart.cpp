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
#include "saves.h"

#define DEBUGLOG Runtime_DEBUGLOG
Log DEBUGLOG = logger.log("Runtime");

#ifdef __PS4__
#define IMAGE_FOLDER "/app0/assets/images"
#define SKIPFRAME_LOG false
#elif __SWITCH__
#define IMAGE_FOLDER "romfs:/images"
#define SKIPFRAME_LOG true
#else
#define IMAGE_FOLDER "../assets/images"
#define SKIPFRAME_LOG true
#endif

SDL_Rect warningArea();
extern SDL_Texture* surface_from_file(std::string path); // From main.cpp lol
SDL_Texture* warningTexture = NULL;
unsigned char memory_snapshot[P8_TOTAL_MEMORY];

LuaState* luaState = NULL;
bool RunningCart::load(Cartridge* cartridge, std::string name)
{
    if (this->status != RunningStatus::None && this->status != RunningStatus::Loaded) {
        DEBUGLOG << "Can't load new cartridge: Already running another one." << ENDL;
        return false;
    }

	if (warningTexture == NULL) {
		DEBUGLOG << "Loading warning image" << ENDL;
		std::string path = IMAGE_FOLDER;
		warningTexture = surface_from_file(path + "/warning.png");
	}

    DEBUGLOG << "Initializing memory" << ENDL;
    memory_load_cartridge(*cartridge);

    audioManager->initialize();
    renderer->initialize();
    machineState->initialize();
    font->initialize();

	DEBUGLOG << "saving initial memory snapshot" << ENDL;
	memcpy(memory_snapshot, p8_memory, P8_TOTAL_MEMORY);

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
	this->name = name;

    return true;
}

void RunningCart::reload(int dest, int source, int length) {
	memcpy(&p8_memory[dest], &memory_snapshot[source], length);
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

	int time_debt = 0;
	short ms_per_frame = millisecs_per_frame(luaState->is60FPS);

	this->status = RunningStatus::Running;
	int frame = 0;
	while (this->status == RunningStatus::Running) {
		frame++;
		machineState->registerFrame();
		audioManager->logStats();

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
					if (!isTimestampNil(this->lastWarning) && getMillisecondsDiff(getTimestamp(), this->lastWarning) > 1000) {
						this->dismissError();
					}

					luaState->run_draw();
					renderer->present();
				}
				else if(SKIPFRAME_LOG) {
					DEBUGLOG << "Skipped frame. time debt = " << time_debt << ENDL;
				}
			}
		}

		if (frame % 30 == 0) {
			saveManager->persist();
		}

		// Get a temptative estimation of how much timeDebt we would have
		auto timediff = getMillisecondsDiff(getTimestamp(), frame_start);
		int remainingTime = ms_per_frame - timediff;
		int tmpTimeDebt = time_debt;
		if (remainingTime > 0) {
			tmpTimeDebt -= remainingTime;
		}
		else {
			tmpTimeDebt += -remainingTime;
		}

		// If it's negative it means that we completed early, wait for that amount of time
		if (tmpTimeDebt < 0) {
			// DEBUGLOG << "Sleep for " << -tmpTimeDebt << ENDL;
			std::this_thread::sleep_for(std::chrono::milliseconds(-tmpTimeDebt));
		}

		// Now update time_debt with the actual difference.
		timediff = getMillisecondsDiff(getTimestamp(), frame_start);
		remainingTime = ms_per_frame - timediff;
		if (remainingTime > 0) {
			time_debt -= remainingTime;
		}
		else {
			time_debt += -remainingTime;
		}

		// Clamp to 2 frames
		if (time_debt > ms_per_frame * 2) {
			time_debt = ms_per_frame * 2;
		}
		else if (time_debt < -ms_per_frame * 2) {
			time_debt = -(ms_per_frame * 2);
		}
	}
}

void RunningCart::warnError() {
	SDL_Rect viewport{};
	SDL_RenderGetViewport(renderer->renderer, &viewport);
	SDL_RenderSetViewport(renderer->renderer, NULL);

	SDL_Rect target = warningArea();
	SDL_SetRenderDrawColor(renderer->renderer, 0x10, 0x10, 0x10, 0xFF);
	SDL_RenderFillRect(renderer->renderer, &target);
	SDL_RenderCopy(renderer->renderer, warningTexture, NULL, &target);

	this->lastWarning = getTimestamp();

	SDL_RenderSetViewport(renderer->renderer, &viewport);
}

std::string RunningCart::getName()
{
	return this->name;
}

void RunningCart::dismissError() {
	SDL_Rect viewport{};
	SDL_RenderGetViewport(renderer->renderer, &viewport);
	SDL_RenderSetViewport(renderer->renderer, NULL);

	SDL_Rect target = warningArea();
	SDL_SetRenderDrawColor(renderer->renderer, 0x10, 0x10, 0x10, 0xFF);
	SDL_RenderFillRect(renderer->renderer, &target);

	this->lastWarning = nilTimestamp();

	SDL_RenderSetViewport(renderer->renderer, &viewport);
}

void blocking_alert(std::string str)
{
	int text_width = str.size() * SYS_CHAR_WIDTH;

	SDL_SetRenderDrawColor(renderer->renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_Rect rect{
		(FRAME_WIDTH - (text_width + 20)) / 2,
		(FRAME_HEIGHT - (SYS_CHAR_HEIGHT + 20)) / 2,
		text_width + 20,
		SYS_CHAR_HEIGHT + 20
	};
	SDL_RenderFillRect(renderer->renderer, &rect);
	SDL_SetRenderDrawColor(renderer->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	font->sys_print(str, rect.x + 10, rect.y + 10);
	SDL_UpdateWindowSurface(renderer->window);

	SDL_Event e;
	bool quit = false;
	while (!quit) {
		if (SDL_PollEvent(&e) == 0) {
			SDL_Delay(100);
			continue;
		}
		if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
			quit = true;
		}

		if (getKeyDown(e) == Key::cross) {
			quit = true;
		}
	}
}

void run_cartridge(Cartridge* r, std::string name)
{
	if (runningCart->load(r, name)) {
		runningCart->run();
		SDL_RenderSetLogicalSize(renderer->renderer, FRAME_WIDTH, FRAME_HEIGHT);
		SDL_RenderSetViewport(renderer->renderer, NULL);
	}
	else {
		SDL_RenderSetLogicalSize(renderer->renderer, FRAME_WIDTH, FRAME_HEIGHT);
		SDL_RenderSetViewport(renderer->renderer, NULL);
		blocking_alert("cartridge not compatible yet");
	}
}

SDL_Rect warningArea() {
	SDL_Rect target;
	SDL_RenderGetViewport(renderer->renderer, &target);
	target.x = target.w;
	target.y = 0;
	SDL_QueryTexture(warningTexture, NULL, NULL, &target.w, &target.h);
	target.w = target.w * 0.2;
	target.h = target.h * 0.2;
	target.x -= target.w + 1;
	target.y = 0.5;

	return target;
}