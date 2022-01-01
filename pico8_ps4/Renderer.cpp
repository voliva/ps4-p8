#include "renderer.h"
#include <vector>;

#include "log.h"
#include <thread>

std::vector<unsigned char> transform_spritesheet_data(std::vector<unsigned char>& input);

#define DEBUGLOG Renderer_DEBUGLOG
Log DEBUGLOG = logger.log("renderer");

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* spritesheet = NULL;

SDL_Rect p8_area{
	0,0,P8_WIDTH,P8_HEIGHT
};

bool init_renderer()
{
	// Initialize SDL functions
	DEBUGLOG << "Initializing SDL" << ENDL;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) != 0)
	{
		DEBUGLOG << "Failed to initialize SDL: " << SDL_GetError() << ENDL;
		return false;
	}

	// Create a window context
	DEBUGLOG << "Creating a window" << ENDL;

	window = SDL_CreateWindow("main", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, FRAME_WIDTH, FRAME_HEIGHT, 0);

	if (!window)
	{
		DEBUGLOG << "Failed to create window: " << SDL_GetError() << ENDL;
		return false;
	}

	// Create a software rendering instance for the window
	SDL_Surface* windowSurface = SDL_GetWindowSurface(window);
	renderer = SDL_CreateSoftwareRenderer(windowSurface);

	if (!renderer)
	{
		DEBUGLOG << "Failed to create software renderer: " << SDL_GetError() << ENDL;
		SDL_Quit();
		return false;
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

    // On PS4 RenderSetClipRect freezes for some reason - workaround will be to manually paint the bounds
	// SDL_RenderSetClipRect(renderer, &p8_area);

	SDL_SetRenderDrawColor(renderer, 0x10, 0x10, 0x10, 0xFF);
	SDL_RenderClear(renderer);

	// Inside area
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	clear_screen();
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_UpdateWindowSurface(window);

	return true;
}

bool renderer_wait_until_ready()
{
	bool success = false;
	bool quit = false;
	SDL_Event e;

	while (!quit && !success) {
		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
				quit = true;
			}

			if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WindowEventID::SDL_WINDOWEVENT_EXPOSED) {
				success = true;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return success;
}

void clear_screen()
{
	SDL_RenderFillRect(renderer, &p8_area);
}

void clip_outside()
{
	unsigned char r, g, b, a;
	SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
	SDL_SetRenderDrawColor(renderer, 0x10, 0x10, 0x10, 0xFF);
	SDL_Rect lower_area {
		0,P8_HEIGHT,2*P8_WIDTH,P8_HEIGHT
	};
	SDL_Rect right_area {
		P8_WIDTH,0,P8_WIDTH,P8_HEIGHT
	};
	SDL_RenderFillRect(renderer, &lower_area);
	SDL_RenderFillRect(renderer, &right_area);
	SDL_SetRenderDrawColor(renderer, r, g,b,a);
}

void load_spritesheet(std::vector<unsigned char>& sprite_map)
{
	spritesheet = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 128, 128);
	SDL_SetTextureBlendMode(spritesheet, SDL_BLENDMODE_BLEND); // Needed for transparency

	std::vector<unsigned char> pixels = transform_spritesheet_data(sprite_map);
	SDL_UpdateTexture(spritesheet, NULL, &pixels[0], 4 * 128);
}

void draw_sprite(int n, int x, int y, int w, int h)
{
	SDL_Rect src{
		(n%16)*8,(n/16)*8,w,h
	};
	SDL_Rect dst{
		x,y,w,h
	};
	SDL_RenderCopy(renderer, spritesheet, &src, &dst);
}

void set_color(int col)
{
	SDL_Color c = PALETTE[col];
	SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
}

#define SPRITESHEET_LENGTH 0x2000
std::vector<unsigned char> transform_spritesheet_data(std::vector<unsigned char>& input)
{
	// TODO palt to change transparency
	std::vector<unsigned char> ret(SPRITESHEET_LENGTH * 2 * 4); // Every byte encodes two pixels, each pixel is 4 bytes in RGBA

	int p = 0;
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