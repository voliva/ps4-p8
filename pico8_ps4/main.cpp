#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <SDL2/SDL.h>
#include <thread>
#include <chrono>
#include <math.h>

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

#define P8_WIDTH 128
#define P8_HEIGHT 128

extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include "Color.h"
#include "font.h"
#include "log.h"
#include "cartridge.h"

#ifdef __PS4__
#define MINEWALKER "/app0/assets/misc/minewalker.p8.png"
#else
#define MINEWALKER "../assets/misc/minewalker.p8.png"
#endif

// SDL window and software renderer
SDL_Window* window;
SDL_Renderer* renderer;

Color bgColor = { 0x10, 0x10, 0x10 };
Color fgColor = { 255, 255, 255 };

static int pong(lua_State* L);

std::vector<unsigned char> transform_spritesheet_data(std::vector<unsigned char>& input);

int main(void)
{
	Font f;
	Log DEBUGLOG = logger.log("main");

	int rc;
	SDL_Surface* windowSurface;

	// No buffering
	setvbuf(stdout, NULL, _IONBF, 0);

	// Initialize SDL functions
	DEBUGLOG << "Initializing SDL" << ENDL;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0)
	{
		DEBUGLOG << "Failed to initialize SDL: " << SDL_GetError() << ENDL;
		return -1;
	}

	// Create a window context
	DEBUGLOG << "Creating a window" << ENDL;

	window = SDL_CreateWindow("main", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, FRAME_WIDTH, FRAME_HEIGHT, 0);

	if (!window)
	{
		DEBUGLOG << "Failed to create window: " << SDL_GetError() << ENDL;
		return -1;
	}

	// Create a software rendering instance for the window
	windowSurface = SDL_GetWindowSurface(window);
	renderer = SDL_CreateSoftwareRenderer(windowSurface);

	if (!renderer)
	{
		DEBUGLOG << "Failed to create software renderer: " << SDL_GetError() << ENDL;
		SDL_Quit();
		return -1;
	}

	// If we make a logical size of P8_HEIGHT, then SDL struggles with decimal scales and adds random black lines.
	// So we need a logical size that divides P8_HEIGHT
	unsigned int scale = FRAME_HEIGHT / P8_HEIGHT;
	// Assuming square, but logic should still work for other aspect ratios (but more complex)
	unsigned int logicalSize = FRAME_HEIGHT / scale;
	SDL_RenderSetLogicalSize(renderer, logicalSize, logicalSize);

	// Center the viewport as much as posible
	SDL_Rect viewport{};
	SDL_RenderGetViewport(renderer, &viewport);
	viewport.y = (viewport.h - P8_HEIGHT) / 2;
	SDL_RenderSetViewport(renderer, &viewport);


	// SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); // => Not needed when playing with transparency https://stackoverflow.com/questions/24241974/sdl2-generate-fully-transparent-texture

	DEBUGLOG << "Loading cartridge..." << ENDL;
	Cartridge* r = load_from_png(MINEWALKER);

	SDL_Texture *spritesheet = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 128, 128);
	SDL_SetTextureBlendMode(spritesheet, SDL_BLENDMODE_BLEND); // Needed for transparency

	std::vector<unsigned char> pixels = transform_spritesheet_data(r->sprite_map);
	SDL_UpdateTexture(spritesheet, NULL, &pixels[0], 4 * 128);

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
	DEBUGLOG << "Entering draw loop..." << ENDL;

	//for (int frame = 0; frame < 1000; frame++)
	//{
	// Clear the canvas
	// Outside area
	SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, 0xFF);
	SDL_RenderClear(renderer);
	// Inside area
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_Rect p8_area{
		0,0,P8_WIDTH,P8_HEIGHT
	};
	SDL_RenderFillRect(renderer, &p8_area);

	SDL_SetRenderDrawColor(renderer, fgColor.r, fgColor.g, fgColor.b, 0xFF);

	f.print("hello world! 🅾️a", 0, 0, renderer);

	SDL_Rect src{
		8,0,8,8
	};
	SDL_Rect dst{
		20,20,8,8
	};
	SDL_RenderCopy(renderer, spritesheet, &src, &dst);

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
			DEBUGLOG << e << ENDL;

			return -1;
		}
	}

	DEBUGLOG << "Calling" << ENDL;

	lua_getglobal(L, "ping");
	lua_pushstring(L, "sent stuff");

	if (lua_pcall(L, 1, 0, 0) != 0) {
		DEBUGLOG << "Failed calling function" << ENDL;
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

	logger << result << ENDL;

	return 0;  /* number of results */
}

SDL_Color PALETTE[] = {
	SDL_Color { 0x00, 0x00, 0x00, 0xff },
	SDL_Color { 0x1d, 0x2b, 0x53, 0xff },
	SDL_Color { 0x7e, 0x25, 0x53, 0xff },
	SDL_Color { 0x00, 0x87, 0x51, 0xff },
	SDL_Color { 0xab, 0x52, 0x36, 0xff },
	SDL_Color { 0x5f, 0x57, 0x4f, 0xff },
	SDL_Color { 0xc2, 0xc3, 0xc7, 0xff },
	SDL_Color { 0xff, 0xf1, 0xe8, 0xff },
	SDL_Color { 0xff, 0x00, 0x4d, 0xff },
	SDL_Color { 0xff, 0xa3, 0x00, 0xff },
	SDL_Color { 0xff, 0xec, 0x27, 0xff },
	SDL_Color { 0x00, 0xe4, 0x36, 0xff },
	SDL_Color { 0x29, 0xad, 0xff, 0xff },
	SDL_Color { 0x83, 0x76, 0x9c, 0xff },
	SDL_Color { 0xff, 0x77, 0xa8, 0xff },
	SDL_Color { 0xff, 0xcc, 0xaa, 0xff },
	SDL_Color { 0xff, 0x00, 0x00, 0xff },
};

#define SPRITESHEET_LENGTH 0x2000
std::vector<unsigned char> transform_spritesheet_data(std::vector<unsigned char>& input)
{
	// TODO palt to change transparency
	std::vector<unsigned char> ret(SPRITESHEET_LENGTH * 2 * 4); // Every byte encodes two pixels, each pixel is 4 bytes in RGBA
	SDL_Color transparent = PALETTE[16];

	int p=0;
	for (int i = 0; i < SPRITESHEET_LENGTH; i++) {
		unsigned char left = input[i] & 0x0F;
		if (left == 0) {
			ret[p++] = 0x00;
			ret[p++] = 0x00;
			ret[p++] = 0x00;
			ret[p++] = 0x00;
		}
		else {
			SDL_Color c = PALETTE[left];
			ret[p++] = c.a;
			ret[p++] = c.b;
			ret[p++] = c.g;
			ret[p++] = c.r;
		}

		unsigned char right = input[i] >> 4;
		if (right == 0) {
			ret[p++] = 0x00;
			ret[p++] = 0x00;
			ret[p++] = 0x00;
			ret[p++] = 0x00;
		}
		else {
			SDL_Color c = PALETTE[right];
			ret[p++] = c.a;
			ret[p++] = c.b;
			ret[p++] = c.g;
			ret[p++] = c.r;
		}
	}

	return ret;
}