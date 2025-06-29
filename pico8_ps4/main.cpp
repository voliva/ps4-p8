#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include "chrono.h"
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
#include "saves.h"
#include "file_paths.h"
#include "system_menu.h"
#include "main.h"
#include "carousel.h"

#if __SWITCH__
#define CAROUSEL_CART_HEIGHT 350
#else
#define CAROUSEL_CART_HEIGHT 400
#endif

#define DEBUGLOG Rom_DEBUGLOG
Log DEBUGLOG = logger.log("ROM");
MachineState *machineState;
AudioManager *audioManager;
Renderer *renderer;
Font *font;
PauseMenu *pauseMenu;
RunningCart *runningCart;
SaveManager *saveManager;

typedef struct
{
	SDL_Texture *surface;
	std::string path;
} LocalCartridge;
typedef struct
{
	std::string name;
	std::vector<LocalCartridge> cartridges;
} Screen;
std::vector<LocalCartridge> load_local_cartridges(std::string directory);

#include "http.h"
#include "splore_loader.h"
#include "splore.h"
#include "save_states.h"

bool isSplore(int screen)
{
	return screen == 2 || screen == 3;
}

std::string name_from_path(std::string path)
{
	std::string filename = path.substr(path.find_last_of("/") + 1);
	return filename.substr(0, filename.length() - 7);
}

#ifdef __SWITCH__
#include <switch.h>

void initSystem()
{
	romfsInit();
	socketInitializeDefault();
}
void closeSystem()
{
	socketExit();
	romfsExit();
	consoleExit(NULL);
}
#else
void initSystem() {}
void closeSystem() {}
#endif

bool quit = false;

int main(void)
{
	initSystem();
	prepareFilePaths();

	DEBUGLOG << "Initializing save states..." << ENDL;
	initialize_save_states();

	DEBUGLOG << "Initializing renderer..." << ENDL;
	renderer = new Renderer();

	DEBUGLOG << "Initializing audio..." << ENDL;
	audioManager = new AudioManager();

	DEBUGLOG << "Initializing state..." << ENDL;
	machineState = new MachineState();
	font = new Font();
	pauseMenu = new PauseMenu();
	runningCart = new RunningCart();
	saveManager = new SaveManager();

	DEBUGLOG << "Initializing http..." << ENDL;
	http_init();

	// Initialize input / joystick
	if (SDL_NumJoysticks() > 0)

	{
		DEBUGLOG << "Initialize joysticks" << ENDL;
		SDL_JoystickOpen(0);
	}

	DEBUGLOG << "Begin rendering" << ENDL;
	SDL_Surface *screenSurface = SDL_GetWindowSurface(renderer->window);

	std::vector<LocalCartridge> bundledCartridges = load_local_cartridges(BUNDLED_FOLDER);
	std::vector<LocalCartridge> localCartridges = load_local_cartridges(CARTRIDGE_FOLDER);
	std::vector<LocalCartridge> fakeBbs;
	fakeBbs.push_back(LocalCartridge{
		NULL, ""});

	Screen screens[] = {
		Screen{
			"bundled",
			bundledCartridges},
		Screen{
			"local",
			localCartridges},
		Screen{
			"bbs featured",
			fakeBbs},
		Screen{
			"bbs new",
			fakeBbs},
	};
	Splore splore;
	int currentScreen = 1;
	if (localCartridges.size() == 0)
	{
		currentScreen = 0;
	}

	Carousel* carousel = new Carousel(screens[currentScreen].cartridges.size(), 160 * ((double)CAROUSEL_CART_HEIGHT / 205), CAROUSEL_CART_HEIGHT);

	SDL_Event e;

	auto frame_start = getTimestamp();
	while (!quit)
	{
		int prevScreen = currentScreen - 1;
		while (prevScreen >= 0 && screens[prevScreen].cartridges.size() == 0)
		{
			prevScreen--;
		}
		bool canDecScreen = prevScreen >= 0;
		int nextScreen = currentScreen + 1;
		while (nextScreen < 4 && screens[nextScreen].cartridges.size() == 0)
		{
			nextScreen++;
		}
		bool canIncScreen = nextScreen < 4;

		if (invalidateLocalCartridges) {
			invalidateLocalCartridges = false;

			localCartridges = load_local_cartridges(CARTRIDGE_FOLDER);
			screens[1].cartridges = localCartridges;
			if (currentScreen == 1) {
				if (localCartridges.size() == 0) {
					currentScreen = 0;

					carousel->reset();
					carousel->setItemcount(screens[currentScreen].cartridges.size());
				}
				else {
					carousel->setItemcount(localCartridges.size());
				}
			}
		}

		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE))
			{
				quit = true;
			}

			Key k = getKeyDown(e);
			switch (k)
			{
			case Key::quit:
				quit = true;
				break;
			case Key::L2:
				if (canDecScreen)
				{
					currentScreen = prevScreen;

					if (currentScreen < 2) {
						carousel->reset();
						carousel->setItemcount(screens[currentScreen].cartridges.size());
					}

					if (currentScreen == 2) {
						splore.initialize(Mode::Featured);
					}
					else if (currentScreen == 3) {
						splore.initialize(Mode::New);
					}
				}
				break;
			case Key::R2:
				if (canIncScreen)
				{
					currentScreen = nextScreen;

					if (currentScreen < 2) {
						carousel->reset();
						carousel->setItemcount(screens[currentScreen].cartridges.size());
					}

					if (currentScreen == 2)
					{
						splore.initialize(Mode::Featured);
					}
					else if (currentScreen == 3)
					{
						splore.initialize(Mode::New);
					}
				}
				break;
			}

			if (activeSystemMenu) {
				activeSystemMenu->keyDown(k);
			} else if (isSplore(currentScreen))
			{
				splore.key_down(k);
			}
			else
			{
				carousel->keyDown(k);
				switch (k)
				{
				case Key::cross: {
					std::string path = screens[currentScreen].cartridges[carousel->getActiveIndex()].path;
					Cartridge* r = load_from_png(path);
					run_cartridge(r, name_from_path(path));
					delete r;
					break;
				}
				case Key::pause: {
					if (currentScreen == 1) {
						std::string path = screens[1].cartridges[carousel->getActiveIndex()].path;
						std::vector<MenuItem> items = {
							MenuItem {
								"Delete cartridge",
								[path]() {
									remove(path.c_str());
									invalidateLocalCartridges = true;
								},
								true
							}
						};
						activeSystemMenu = new SystemMenu(items);
					}
					break;
				}
				}
			}
		}

		auto now = getTimestamp();
		auto delta = getMillisecondsDiff(now, frame_start);
		frame_start = now;

		Screen scr = screens[currentScreen];

		SDL_SetRenderDrawColor(renderer->renderer, 0x10, 0x10, 0x10, 0xFF);
		SDL_RenderClear(renderer->renderer);
		SDL_SetRenderDrawColor(renderer->renderer, 0xFF, 0xFF, 0xFF, 0xFF);

		font->sys_print(scr.name, (FRAME_WIDTH - SYS_CHAR_WIDTH * scr.name.length()) / 2, 30);

		if (canDecScreen)
		{
#ifdef __SWITCH__
			font->sys_print("<lz", FRAME_WIDTH / 4 - 3 * SYS_CHAR_WIDTH, 30);
#else
			font->sys_print("<l2", FRAME_WIDTH / 3 - 3 * SYS_CHAR_WIDTH, 30);
#endif
		}
		if (canIncScreen)
		{
#ifdef __SWITCH__
			font->sys_print("rz>", 3 * FRAME_WIDTH / 4, 30);
#else
			font->sys_print("r2>", 2 * FRAME_WIDTH / 3, 30);
#endif
		}

		if (isSplore(currentScreen))
		{
			splore.render();
		}
		else
		{
			auto items = carousel->draw(delta);

			for (int i = 0; i < items.size(); i++) {
				auto item = items[i];
				if (item.idx < 0 || item.idx >= scr.cartridges.size()) continue;

				SDL_Texture* srf = scr.cartridges[item.idx].surface;
				SDL_SetTextureAlphaMod(srf, item.alpha);
				SDL_RenderCopy(renderer->renderer, srf, NULL, &item.destRect);
			}

			double idx = carousel->getActiveIndex();
			if (scr.cartridges.size() > idx) {
				std::string currentPath = scr.cartridges[idx].path;
				std::string name = name_from_path(currentPath);
				font->sys_print(
					name,
					(FRAME_WIDTH - SYS_CHAR_WIDTH * name.length()) / 2,
					30 + SYS_CHAR_HEIGHT + 100 + CAROUSEL_CART_HEIGHT + 100);
			}
		}

		if (activeSystemMenu) {
			activeSystemMenu->draw();
		}

		std::string github = "github.com/voliva/ps4-p8";
		font->sys_print(
			github,
			FRAME_WIDTH - SYS_CHAR_WIDTH * github.length() / 3,
			FRAME_HEIGHT - SYS_CHAR_HEIGHT / 3,
			0.33);

		SDL_UpdateWindowSurface(renderer->window);
	}

	delete audioManager;
	SDL_Quit();
	closeSystem();

	return 0;
}

SDL_Texture *surface_from_file(std::string path)
{
	int width, height, channels;
	unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

	// For some reason stbi_load gives rgb_a as [a,b,g,r] stream
	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(data, width, height, 4 * 8, width * 4, 0x0000000FF, 0x00000FF00, 0x000FF0000, 0x0FF000000);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer->renderer, surface);
	SDL_FreeSurface(surface);

	return texture;
}
std::vector<LocalCartridge> load_local_cartridges(std::string directory)
{
	std::vector<LocalCartridge> result;

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(directory.c_str())) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			std::string filename = ent->d_name;
			if (filename.find(".p8.png") != std::string::npos)
			{
				std::string path = directory + "/" + filename;
				SDL_Texture *surface = surface_from_file(path);
				if (surface != NULL)
				{
					result.push_back({surface,
									  path});
				}
			}
		}
		closedir(dir);
	}
	else
	{
		std::string err = strerror(errno);
		DEBUGLOG << "Can't open dir " << directory << ": " << err << ENDL;
	}

	return result;
}
