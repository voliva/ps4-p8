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
#include "pause_menu.h"
#include "running-cart.h"
#include "performance_monitor.h"

#ifdef __PS4__
#define BUNDLED_FOLDER "/app0/assets/misc"
#define CARTRIDGE_FOLDER "/data/p8-cartridges"
#else
#define BUNDLED_FOLDER "../assets/misc"
#define CARTRIDGE_FOLDER "../p8-cartridges"
#endif

#define CAROUSEL_CART_HEIGHT 400

#define DEBUGLOG Rom_DEBUGLOG
Log DEBUGLOG = logger.log("ROM");
MachineState* machineState;
AudioManager* audioManager;
Renderer* renderer;
Font* font;
PauseMenu* pauseMenu;
RunningCart* runningCart;
PerformanceMonitor* performanceMonitor;

typedef struct {
	SDL_Texture* surface;
	std::string path;
} LocalCartridge;
typedef struct {
	std::string name;
	std::vector<LocalCartridge> cartridges;
} Screen;
std::vector<LocalCartridge> load_local_cartridges(std::string directory);

int main(void)
{
	DEBUGLOG << "Initializing renderer..." << ENDL;
	renderer = new Renderer();

	DEBUGLOG << "Initializing audio..." << ENDL;
	audioManager = new AudioManager();

	machineState = new MachineState();
	font = new Font();
	pauseMenu = new PauseMenu();
	runningCart = new RunningCart();
	performanceMonitor = new PerformanceMonitor();

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

	auto frame_start = std::chrono::high_resolution_clock::now();
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
				runningCart->load(r);
				runningCart->run();
				delete r;
				SDL_RenderSetLogicalSize(renderer->renderer, FRAME_WIDTH, FRAME_HEIGHT);
				SDL_RenderSetViewport(renderer->renderer, NULL);
				break;
			}
		}

		auto now = std::chrono::high_resolution_clock::now();
		auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - frame_start).count();
		frame_start = now;

		double target_diff = selectedCart - renderingTargetCart;
		double movement = target_diff * 0.1 * delta / 15;
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
			double rendering_diff = fabs(renderingTargetCart - i);
			if (rendering_diff > 2.2) {
				continue;
			}

			SDL_Texture* srf = scr.cartridges[i].surface;
			int width = 160 * ((double)CAROUSEL_CART_HEIGHT / 205);

			int alpha = 255 - 255 * rendering_diff / 2.2;
			if (alpha < 0) alpha = 0;
			SDL_SetTextureAlphaMod(srf, alpha);

			double scale = 1.2 - 0.2 * rendering_diff;
			if (scale < 1) scale = 1;

			int x_center = FRAME_WIDTH / 2 + (width + 100) * ((double)i - renderingTargetCart);
			double x = x_center - width * scale / 2;
			double y = 30 + SYS_CHAR_HEIGHT + 100 + (1 - scale) * CAROUSEL_CART_HEIGHT / 2;
			double w = (double)width * scale;
			double h = (double)CAROUSEL_CART_HEIGHT * scale;

			SDL_Rect dest {
				(int)x,
				(int)y,
				(int)w,
				(int)h
			};
			SDL_RenderCopy(renderer->renderer, srf, NULL, &dest);
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
	}

	SDL_Quit();

	return 0;
}

SDL_Texture* surface_from_file(std::string path) {
	int width, height, channels;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

	// For some reason stbi_load gives rgb_a as [a,b,g,r] stream
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(data, width, height, 4 * 8, width * 4, 0x0000000FF, 0x00000FF00, 0x000FF0000, 0x0FF000000);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer->renderer, surface);
	SDL_FreeSurface(surface);

	return texture;
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
				SDL_Texture* surface = surface_from_file(path);
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
