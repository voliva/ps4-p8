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
#include "memory.h"
#include "font.h"
#include <stb/stb_image.h>
#include <dirent.h>

#ifdef __PS4__
#define BUNDLED_FOLDER "/app0/assets/misc"
#define CARTRIDGE_FOLDER "/data/p8-cartridges"
#else
#define BUNDLED_FOLDER "../assets/misc"
#define CARTRIDGE_FOLDER "../p8-cartridges"
#endif

#define CAROUSEL_CART_HEIGHT 400

int millisecs_per_frame(bool is60Fps);

#define DEBUGLOG Rom_DEBUGLOG
Log DEBUGLOG = logger.log("ROM");
MachineState* machineState;
AudioManager* audioManager;
Renderer* renderer;
Font* font;

typedef struct {
	SDL_Surface* surface;
	std::string path;
} LocalCartridge;
typedef struct {
	std::string name;
	std::vector<LocalCartridge> cartridges;
} Screen;
std::vector<LocalCartridge> load_local_cartridges(std::string directory);
bool run_cartridge(Cartridge *r);

int main(void)
{
	DEBUGLOG << "Initializing renderer..." << ENDL;
	renderer = new Renderer();

	DEBUGLOG << "Initializing audio..." << ENDL;
	audioManager = new AudioManager();

	machineState = new MachineState();
	font = new Font();

	// Initialize input / joystick
	if (SDL_NumJoysticks() > 0)
	{
		DEBUGLOG << "Initialize joysticks" << ENDL;
		SDL_JoystickOpen(0);
	}

	SDL_Surface* screenSurface = SDL_GetWindowSurface(renderer->window);

	std::vector<LocalCartridge> bundledCartridges = load_local_cartridges(BUNDLED_FOLDER);
	std::vector<LocalCartridge> localCartridges = load_local_cartridges(CARTRIDGE_FOLDER);

	Screen screens[] = {
		Screen{
			"bundled",
			bundledCartridges
		},
		Screen{
			"local",
			localCartridges
		},
		// TODO BBS
	};
	int currentScreen = 1;
	if (localCartridges.size() == 0) {
		currentScreen = 0;
	}
	int selectedCart = 0;
	double renderingTargetCart = 0;

	SDL_Event e;
	bool quit = false;
	while (!quit) {
		int prevScreen = currentScreen - 1;
		while (prevScreen >= 0 && screens[prevScreen].cartridges.size() == 0) {
			prevScreen--;
		}
		bool canDecScreen = prevScreen >= 0;
		int nextScreen = currentScreen + 1;
		while (nextScreen < 2 && screens[nextScreen].cartridges.size() == 0) {
			nextScreen++;
		}
		bool canIncScreen = nextScreen < 2;

		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
				quit = true;
			}

			switch (getKeyDown(e)) {
			case Key::L2:
				if (canDecScreen) {
					currentScreen = prevScreen;
					selectedCart = 0;
					renderingTargetCart = 0;
				}
				break;
			case Key::R2:
				if (canIncScreen) {
					currentScreen = nextScreen;
					selectedCart = 0;
					renderingTargetCart = 0;
				}
				break;
			case Key::left:
				if (selectedCart == 0) break;
				selectedCart--;
				break;
			case Key::right:
				if (selectedCart == screens[currentScreen].cartridges.size() - 1) break;
				selectedCart++;
				break;
			case Key::cross:
				Cartridge* r = load_from_png(screens[currentScreen].cartridges[selectedCart].path);
				run_cartridge(r);
				delete r;
				SDL_RenderSetLogicalSize(renderer->renderer, FRAME_WIDTH, FRAME_HEIGHT);
				SDL_RenderSetViewport(renderer->renderer, NULL);
				break;
			}
		}

		double target_diff = selectedCart - renderingTargetCart;
		double movement = target_diff * 0.1;
		if (movement > 0) {
			if (movement < 0.01) {
				movement = 0.01;
			}
			if (renderingTargetCart + movement > selectedCart) {
				movement = selectedCart - renderingTargetCart;
			}
		}
		else if(movement < 0) {
			if (movement > -0.01) {
				movement = -0.01;
			}
			if (renderingTargetCart + movement < selectedCart) {
				movement = selectedCart - renderingTargetCart;
			}
		}
		renderingTargetCart += movement;

		Screen scr = screens[currentScreen];

		SDL_SetRenderDrawColor(renderer->renderer, 0x10, 0x10, 0x10, 0xFF);
		SDL_RenderClear(renderer->renderer);
		SDL_SetRenderDrawColor(renderer->renderer, 0xFF, 0xFF, 0xFF, 0xFF);

		font->sys_print(scr.name, (FRAME_WIDTH - SYS_CHAR_WIDTH * scr.name.length()) / 2, 30);

		if (canDecScreen) {
			font->sys_print("<l2", FRAME_WIDTH / 3 - 3*SYS_CHAR_WIDTH, 30);
		}
		if (canIncScreen) {
			font->sys_print("r2>", 2 * FRAME_WIDTH / 3, 30);
		}

		for (int i = 0; i < scr.cartridges.size(); i++) {
			SDL_Surface* srf = scr.cartridges[i].surface;
			int width = srf->w * ((double)CAROUSEL_CART_HEIGHT / srf->h);

			double scale = 1.2 - 0.2 * abs(renderingTargetCart - i);
			if (scale < 1) scale = 1;

			int x_center = FRAME_WIDTH / 2 + (width + 100) * ((double)i - renderingTargetCart);
			SDL_Rect dest {
				(int)(x_center - width * scale / 2),
				(int)(30 + SYS_CHAR_HEIGHT + 100 + (1-scale) * CAROUSEL_CART_HEIGHT / 2),
				(int)(width * scale),
				(int)(CAROUSEL_CART_HEIGHT * scale)
			};
			SDL_BlitScaled(srf, NULL, screenSurface, &dest);
		}
		std::string currentPath = scr.cartridges[round(renderingTargetCart)].path;
		std::string filename = currentPath.substr(currentPath.find_last_of("/") + 1);
		std::string name = filename.substr(0, filename.length() - 7);
		font->sys_print(
			name,
			(FRAME_WIDTH - SYS_CHAR_WIDTH * name.length()) / 2,
			30 + SYS_CHAR_HEIGHT + 100 + CAROUSEL_CART_HEIGHT + 100
		);

		SDL_UpdateWindowSurface(renderer->window);

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	SDL_Quit();

	return 0;
}

SDL_Surface* surface_from_file(std::string path) {
	int width, height, channels;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	// For some reason stbi_load gives rgb_a as [a,b,g,r] stream
	return SDL_CreateRGBSurfaceFrom(data, width, height, 4*8, width * 4, 0x0000000FF, 0x00000FF00, 0x000FF0000, 0x0FF000000);
}
std::vector<LocalCartridge> load_local_cartridges(std::string directory) {
	std::vector<LocalCartridge> result;

	DIR* dir;
	struct dirent* ent;
	if ((dir = opendir(directory.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			std::string filename = ent->d_name;
			if (filename.find(".p8.png") != std::string::npos) {
				std::string path = directory + "/" + filename;
				SDL_Surface* surface = surface_from_file(path);
				if (surface != NULL) {
					result.push_back({
						surface,
						path
					});
				}
			}
		}
		closedir(dir);
	}
	else {
		std::string err = strerror(errno);
		DEBUGLOG << "Can't open dir " << directory << ": " << err << ENDL;
	}

	return result;
}

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