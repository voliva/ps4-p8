#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <SDL2/SDL.h>
#include <thread>
#include <chrono>

#ifdef __PS4__
#include <orbis/libkernel.h>
#include <orbis/Sysmodule.h>

#define FRAME_WIDTH     1920
#define FRAME_HEIGHT    1080
#else
// SDL messes up with the linker somehow on windows?
#undef main
#define FRAME_WIDTH     1440
#define FRAME_HEIGHT    810
#endif

extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include "Color.h"
#include "font.h"
#include "log.h"

// Logging
Logger logger;

// SDL window and software renderer
SDL_Window* window;
SDL_Renderer* renderer;

Color bgColor = { 0x10, 0x10, 0x10 };
Color fgColor = { 255, 255, 255 };

static int pong(lua_State* L);

int main(void)
{
	Font f;
	Log DEBUGLOG = logger.log("main");

	int rc;
	SDL_Surface* windowSurface;

	// No buffering
	setvbuf(stdout, NULL, _IONBF, 0);

	// Initialize SDL functions
	DEBUGLOG << "Initializing SDL";

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0)
	{
		DEBUGLOG << "Failed to initialize SDL: " << SDL_GetError();
		return -1;
	}

	// Create a font face for debug and score text
	const char* debugFontPath = "/app0/assets/fonts/VeraMono.ttf";

	// Create a window context
	DEBUGLOG << "Creating a window";

	window = SDL_CreateWindow("main", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, FRAME_WIDTH, FRAME_HEIGHT, 0);

	if (!window)
	{
		DEBUGLOG << "Failed to create window: " << SDL_GetError();
		return -1;
	}

	// Create a software rendering instance for the window
	windowSurface = SDL_GetWindowSurface(window);
	renderer = SDL_CreateSoftwareRenderer(windowSurface);

	if (!renderer)
	{
		DEBUGLOG << "Failed to create software renderer: " << SDL_GetError();
		SDL_Quit();
		return -1;
	}

	// Initialize input / joystick
	/*if (SDL_NumJoysticks() < 1)
	{
		DEBUGLOG << "No controllers available!";
		return -1;
	}

	SDL_Joystick *controller = SDL_JoystickOpen(0);

	if (controller == NULL)
	{
		DEBUGLOG << "Couldn't open controller handle: " << SDL_GetError();
		return -1;
	}*/

	// Enter the render loop
	DEBUGLOG << "Entering draw loop...";

	//for (int frame = 0; frame < 1000; frame++)
	//{
	// Clear the canvas
	SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, 0xFF);
	SDL_RenderClear(renderer);

	// Run all rendering routines
	// render(renderer);

	// Propagate the updated window to the screen
	SDL_UpdateWindowSurface(window);

	// Run all update routines
	//update(renderer, deltaFrameTicks, frameCounter++, startFrameTicks);
	//}

	int error;
	lua_State* L = luaL_newstate();   // opens Lua *
	/*luaopen_base(L);             // opens the basic library *
	luaopen_table(L);            // opens the table library *
	luaopen_io(L);               // opens the I/O library *
	luaopen_string(L);           // opens the string lib. *
	luaopen_math(L);             // opens the math lib. * */
	lua_pushcfunction(L, pong);
	lua_setglobal(L, "pong");

	std::vector<std::string> lines = {
		"function ping(v) \n\
		pong(v)\n \
		end"
	};

	for (int i = 0; i < lines.size(); i++) {
		error = luaL_loadbuffer(L, lines[i].c_str(), lines[i].length(), "line") ||
			lua_pcall(L, 0, 0, 0);
		if (error) {
			std::string e = lua_tostring(L, -1);
			lua_pop(L, 1);  // pop error message from the stack *
			DEBUGLOG << e.c_str();

			return -1;
		}
	}

	DEBUGLOG << "Calling";

	lua_getglobal(L, "ping");
	lua_pushstring(L, "sent stuff");

	if (lua_pcall(L, 1, 0, 0) != 0) {
		DEBUGLOG << "Failed calling function";
	}

	lua_close(L);

	SDL_Event e;
	bool quit = false;
	while (!quit) {
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	SDL_Quit();

	return 0;
}

static int pong(lua_State* L) {
	std::string result = lua_tostring(L, 1);  /* get argument */

	logger << result;

	return 0;  /* number of results */
}

// void *L = (void *)(luaL_newstate);
/**/
