#include "renderer.h"
#include <vector>
#include "memory.h"
#include "log.h"
#include <thread>

std::vector<unsigned char> transform_spritesheet_data(std::vector<unsigned char>& input);

#define DEBUGLOG Renderer_DEBUGLOG
Log DEBUGLOG = logger.log("renderer");

SDL_Rect p8_area {
	0,0,P8_WIDTH,P8_HEIGHT
};

SDL_Color DEFAULT_PALETTE[] = {
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
};
SDL_Color EXTENDED_PALETTE[] = {
	SDL_Color { 0x29, 0x18, 0x14, 0xff },
	SDL_Color { 0x11, 0x1D, 0x35, 0xff },
	SDL_Color { 0x42, 0x21, 0x36, 0xff },
	SDL_Color { 0x12, 0x53, 0x59, 0xff },
	SDL_Color { 0x74, 0x2F, 0x29, 0xff },
	SDL_Color { 0x49, 0x33, 0x3B, 0xff },
	SDL_Color { 0xA2, 0x88, 0x79, 0xff },
	SDL_Color { 0xF3, 0xEF, 0x7D, 0xff },
	SDL_Color { 0xBE, 0x12, 0x50, 0xff },
	SDL_Color { 0xFF, 0x6C, 0x24, 0xff },
	SDL_Color { 0xA8, 0xE7, 0x2E, 0xff },
	SDL_Color { 0x00, 0xB5, 0x43, 0xff },
	SDL_Color { 0x06, 0x5A, 0xB5, 0xff },
	SDL_Color { 0x75, 0x46, 0x65, 0xff },
	SDL_Color { 0xFF, 0x6E, 0x59, 0xff },
	SDL_Color { 0xFF, 0x9D, 0x81, 0xff },
};

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
			SDL_Color c = DEFAULT_PALETTE[left];
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
			SDL_Color c = DEFAULT_PALETTE[right];
			ret[p++] = c.a;
			ret[p++] = c.b;
			ret[p++] = c.g;
			ret[p++] = c.r;
		}
	}

	return ret;
}

Renderer::Renderer()
{
	// Initialize SDL functions
	DEBUGLOG << "Initializing SDL" << ENDL;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) != 0)
	{
		DEBUGLOG << "Failed to initialize SDL: " << SDL_GetError() << ENDL;
		exit(1);
	}

	// Create a window context
	DEBUGLOG << "Creating a window" << ENDL;

	this->window = SDL_CreateWindow("main", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, FRAME_WIDTH, FRAME_HEIGHT, 0);

	if (!this->window)
	{
		DEBUGLOG << "Failed to create window: " << SDL_GetError() << ENDL;
		exit(1);
	}

	// Create a software rendering instance for the window
	SDL_Surface* windowSurface = SDL_GetWindowSurface(this->window);
	this->renderer = SDL_CreateSoftwareRenderer(windowSurface);

	if (!this->renderer)
	{
		DEBUGLOG << "Failed to create software renderer: " << SDL_GetError() << ENDL;
		SDL_Quit();
		exit(1);
	}

	// If we make a logical size of P8_HEIGHT, then SDL struggles with decimal scales and adds random black lines.
	// So we need a logical size that divides P8_HEIGHT
	unsigned int scale = FRAME_HEIGHT / P8_HEIGHT;
	// Assuming square, but logic should still work for other aspect ratios (but more complex)
	unsigned int logicalSize = FRAME_HEIGHT / scale;
	SDL_RenderSetLogicalSize(this->renderer, logicalSize, logicalSize);

	// Center the viewport as much as posible
	SDL_Rect viewport{};
	SDL_RenderGetViewport(this->renderer, &viewport);
	viewport.y = (viewport.h - P8_HEIGHT) / 2;
	SDL_RenderSetViewport(this->renderer, &viewport);

	// On PS4 RenderSetClipRect freezes for some reason - workaround will be to manually paint the bounds
	// SDL_RenderSetClipRect(renderer, &p8_area);

	SDL_SetRenderDrawColor(this->renderer, 0x10, 0x10, 0x10, 0xFF);
	SDL_RenderClear(this->renderer);

	// Inside area
	SDL_SetRenderDrawColor(this->renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderFillRect(this->renderer, &p8_area);
	SDL_SetRenderDrawColor(this->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_UpdateWindowSurface(this->window);

}

void Renderer::initialize()
{
	char transp_mask = 0x10; // Color 0 = black set to transparent
	for (int i = 0; i < 16; i++) {
		p8_memory[ADDR_DS_DRAW_PAL + i] = i | transp_mask;
		p8_memory[ADDR_DS_SCREEN_PAL + i] = i;
		transp_mask = 0; // Only the first mask
	}

	p8_memory[ADDR_DS_CLIP_RECT] = 0;
	p8_memory[ADDR_DS_CLIP_RECT + 1] = 0;
	p8_memory[ADDR_DS_CLIP_RECT + 2] = 128;
	p8_memory[ADDR_DS_CLIP_RECT + 3] = 128;

	p8_memory[ADDR_DS_COLOR] = 6;

	memory_write_short(ADDR_DS_CAMERA_X, 0);
	memory_write_short(ADDR_DS_CAMERA_Y, 0);
}

void Renderer::clear_screen(unsigned char color)
{
	// Every byte in the screen data is two pixels, we want to color the both of them with that color.
	color = (color & 0x0F) | (color << 4);
	memset(&p8_memory[ADDR_SCREEN], color, 0x2000);
}

#define LINE_JMP 128 / 2
void Renderer::draw_sprite(int n, int x, int y, int w, int h)
{
	// I can't use memcpy because this is all effected by the draw state (pal, palt, clip, camera, etc.)

	int sprite_col = n % 16;
	int sprite_row = n / 16;
	int sprite_addr = ADDR_SPRITE_SHEET + sprite_row * LINE_JMP * 8 + sprite_col * 8 / 2;
	for (int _y = 0; _y < h; _y++) {
		for (int _x = 0; _x < w; _x++) {
			unsigned char color = p8_memory[sprite_addr + _y * LINE_JMP + _x / 2];
			if (_x % 2 == 0) {
				color = color & 0x0F;
			}
			else {
				color = color >> 4;
			}
			this->set_transform_pixel(x + _x, y + _y, color);
		}
	}
}

void Renderer::draw_point(int x, int y)
{
}

void Renderer::draw_points(std::vector<Renderer_Point>& points)
{
}

void Renderer::draw_line(int x0, int y0, int x1, int y1)
{
}

void Renderer::draw_circle(int x, int y, int radius, bool fill)
{
}

void Renderer::draw_rectangle(int x0, int y0, int x1, int y1, bool fill)
{
}

void Renderer::present()
{
	for (int i = 0; i < 0x2000; i++) {
		int x = (i%64)*2;
		int y = i/64;
		unsigned char pixels = p8_memory[ADDR_SCREEN + i];

		unsigned char left = p8_memory[ADDR_DS_SCREEN_PAL + (pixels & 0x0F)];
		SDL_Color leftColor = DEFAULT_PALETTE[left & 0x0F];
		if (left >= 0x10) {
			leftColor = EXTENDED_PALETTE[left & 0x0F];
		}
		SDL_SetRenderDrawColor(this->renderer, leftColor.r, leftColor.g, leftColor.b, 0xFF);
		SDL_RenderDrawPoint(this->renderer, x, y);

		unsigned char right = p8_memory[ADDR_DS_SCREEN_PAL + (pixels >> 4)];
		SDL_Color rightColor = DEFAULT_PALETTE[right & 0x0F];
		if (right >= 0x10) {
			rightColor = EXTENDED_PALETTE[right & 0x0F];
		}
		SDL_SetRenderDrawColor(this->renderer, rightColor.r, rightColor.g, rightColor.b, 0xFF);
		SDL_RenderDrawPoint(this->renderer, x+1, y);
	}
	SDL_UpdateWindowSurface(this->window);
}

void Renderer::set_transform_pixel(int x, int y, unsigned char color)
{
	unsigned char mapped_color = p8_memory[ADDR_DS_DRAW_PAL + color];
	// Skip if it's transparent
	if (mapped_color >= 0x10) {
		return;
	}
	mapped_color = mapped_color & 0x0F;

	int screen_x = x + p8_memory[ADDR_DS_CAMERA_X];
	int screen_y = y + p8_memory[ADDR_DS_CAMERA_X];
	unsigned char x0 = p8_memory[ADDR_DS_CLIP_RECT];
	unsigned char y0 = p8_memory[ADDR_DS_CLIP_RECT+1];
	unsigned char x1 = p8_memory[ADDR_DS_CLIP_RECT+2];
	unsigned char y1 = p8_memory[ADDR_DS_CLIP_RECT+3];
	if (x0 <= screen_x && screen_x < x1 &&
		y0 <= screen_y && screen_y < y1) {
		int addr = ADDR_SCREEN + screen_y * LINE_JMP + screen_x / 2;
		if (screen_x % 2 == 0) {
			p8_memory[addr] = (p8_memory[addr] & 0xF0) | mapped_color;
		}
		else {
			p8_memory[addr] = (p8_memory[addr] & 0x0F) | (mapped_color << 4);
		}
	}
}
