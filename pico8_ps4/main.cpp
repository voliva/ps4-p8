#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <thread>

#include "log.h"
#include "cartridge.h"
#include "renderer.h"
#include "events.h"

#ifdef __PS4__
#define MINEWALKER "/app0/assets/misc/minewalker.p8.png"
#else
#define MINEWALKER "../assets/misc/minewalker.p8.png"
#endif

// TODO move out

#define DEBUGLOG Main_DEBUGLOG
Log DEBUGLOG = logger.log("main");
int main(void)
{
	if (!init_renderer()) {
		return -1;
	}

	DEBUGLOG << "Loading cartridge..." << ENDL;
	Cartridge* r = load_from_png(MINEWALKER);

	load_spritesheet(r->sprite_map);

	// Initialize input / joystick
	if (SDL_NumJoysticks() > 0)
	{
		SDL_JoystickOpen(0);
	}

	// Enter the render loop
	DEBUGLOG << "Entering render loop..." << ENDL;

	SDL_Event e;
	bool quit = false;
	while (!quit) {
		while (SDL_PollEvent(&e) != 0)
		{
			// DEBUGLOG << e.type << ENDL;

			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
				quit = true;
			}

			KeyEvent* result = mapSdlEvent(e);

			if (result != NULL) {
				DEBUGLOG << (int)result->key << ENDL;
			}

			delete result;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	SDL_Quit();

	return 0;
}
