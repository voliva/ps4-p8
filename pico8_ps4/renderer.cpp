#include "renderer.h"
#include <vector>
#include "memory.h"
#include "log.h"
#include <thread>
#include "efla.e.h"
#include <math.h>
#include <functional>

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
}

void Renderer::initialize()
{
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

	SDL_SetRenderDrawColor(this->renderer, 0x10, 0x10, 0x10, 0xFF);
	SDL_RenderClear(this->renderer);

	// Inside area
	SDL_SetRenderDrawColor(this->renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderFillRect(this->renderer, &p8_area);
	SDL_SetRenderDrawColor(this->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_UpdateWindowSurface(this->window);

	this->reset_draw_pal();
	this->reset_screen_pal();
	memset(this->prev_screen, 0x00, SCREEN_MEMORY_SIZE);
	memcpy(this->prev_screen_pal, &p8_memory[ADDR_DS_SCREEN_PAL], 16);

	p8_memory[ADDR_DS_CLIP_RECT] = 0;
	p8_memory[ADDR_DS_CLIP_RECT + 1] = 0;
	p8_memory[ADDR_DS_CLIP_RECT + 2] = 127;
	p8_memory[ADDR_DS_CLIP_RECT + 3] = 127;

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

void Renderer::draw_map(int cx, int cy, int sx, int sy, int cw, int ch, unsigned char layer)
{
	for (int y = 0; y < ch; y++) {
		int row = cy + y;
		if (row < 0 || row >= 64) continue;

		int row_offset = ADDR_MAP + row * 128;
		if (row >= 32) {
			row_offset = ADDR_MAP_SHARED + (row-32) * 128;
		}
		int y_pos = sy + y * 8;
		int screen_y = y_pos - (short)memory_read_short(ADDR_DS_CAMERA_Y);
		if (!is_y_drawable(screen_y) && !is_y_drawable(screen_y + 8)) {
			continue;
		}

		for (int x = 0; x < cw; x++) {
			int col = cx + x;
			if (col >= 128 || col < 0) {
				continue;
			}

			int n = p8_memory[row_offset + col];
			if (n == 0) {
				continue;
			}
			int x_pos = sx + x * 8;

			unsigned char flags = p8_memory[ADDR_SPRITE_FLAGS + n];
			bool passes_filter = (unsigned char)(~layer | flags) == 0xFF;
			if (passes_filter) {
				this->draw_sprite(n, x_pos, y_pos, 8, 8, false, false);
			}
		}
	}
}

#define LINE_JMP 128 / 2
void Renderer::draw_sprite(int n, int x, int y, int w, int h, bool flip_x, bool flip_y)
{
	int sprite_col = n % 16;
	int sprite_row = n / 16;

	this->draw_from_spritesheet(sprite_col * 8, sprite_row * 8, w, h, x, y, w, h, flip_x, flip_y);
}

void Renderer::draw_from_spritesheet(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, bool flip_x, bool flip_y)
{
	if (!this->will_be_drawn(dx, dy, dw, dh)) {
		return;
	}

	int sprite_addr = ADDR_SPRITE_SHEET + sy * LINE_JMP + sx / 2;
	int s_offset = sx % 2;
	for (int _y = 0; _y < dh; _y++) {
		int sprite_y = _y * sh / dh;
		if (flip_y) {
			sprite_y = sh - sprite_y - 1;
		}
		for (int _x = s_offset; _x < dw + s_offset; _x++) {
			int sprite_x = _x * sw / dw;
			if (flip_x) {
				sprite_x = sw - sprite_x - 1;
			}

			unsigned char color = p8_memory[sprite_addr + sprite_y * LINE_JMP + sprite_x / 2];
			if (sprite_x % 2 == 0) {
				color = color & 0x0F;
			}
			else {
				color = color >> 4;
			}

			unsigned char mapped_color = this->get_screen_color(color);
			// Skip if it's transparent
			if (mapped_color >= 0x10) {
				continue;
			}
			Renderer_Point sc = this->coord_to_screen(dx + _x, dy + _y);
			if(this->is_drawable(sc.x, sc.y)) {
				this->set_pixel(sc.x, sc.y, mapped_color);
			}
		}
	}
}

void Renderer::draw_point(int x, int y)
{
	unsigned char color = p8_memory[ADDR_DS_COLOR];
	Renderer_Point sc = this->coord_to_screen(x, y);
	unsigned char mapped_color = this->get_screen_pat_color(color, sc.x, sc.y);
	if (mapped_color > 0x0F) {
		// Ignore transparent
		return;
	}
	if (this->is_drawable(sc.x, sc.y)) {
		this->set_pixel(sc.x, sc.y, mapped_color);
	}
}

void Renderer::draw_points(std::vector<Renderer_Point>& points)
{
	for (int i = 0; i < points.size(); i++) {
		this->draw_point(points[i].x, points[i].y);
	}
}

void Renderer::draw_line(int x0, int y0, int x1, int y1)
{
	if (y0 == y1) {
		int xmin = std::min(x0, x1);
		int xmax = std::max(x0, x1);
		Renderer_Point sc_min = this->coord_to_screen(xmin, y0);
		Renderer_Point sc_max = this->coord_to_screen(xmax, y0);
		return this->set_line(sc_min.x, sc_max.x, sc_min.y);
	}
	std::vector<Renderer_Point> points = efla_small_line(x0, y0, x1, y1);
	this->draw_points(points);
}

void Renderer::draw_textured_line(int x0, int y0, int x1, int y1, float mx, float my, float mdx, float mdy)
{
	Renderer_Point sc0 = this->coord_to_screen(x0, y0);
	Renderer_Point sc1 = this->coord_to_screen(x1, y1);

	std::vector<Renderer_Point> points = efla_small_line(sc0.x, sc0.y, sc1.x, sc1.y);
	for (int i = 0; i < points.size(); i++, mx += mdx, my += mdy) {
		if (mx < 0 || mx >= 128) continue;
		if (my < 0 || my >= 64) continue;

		if (this->is_drawable(points[i].x, points[i].y)) {
			int row_offset = ADDR_MAP + (int)my * 128;
			if (my >= 32) {
				row_offset = ADDR_MAP_SHARED + ((int)my - 32) * 128;
			}
			int spr = p8_memory[row_offset + (int)mx];
			if (spr == 0) {
				continue;
			}

			int sx = 8 * (mx - (int)mx);
			int sy = 8 * (my - (int)my);

			int px = (spr % 16) * 8 + sx;
			int py = (spr / 16) * 8 + sy;

			unsigned char color = p8_memory[ADDR_SPRITE_SHEET + py * LINE_JMP + px / 2];
			if (px % 2 == 0) {
				color = color & 0x0F;
			}
			else {
				color = color >> 4;
			}

			unsigned char mapped_color = this->get_screen_color(color);
			// Skip if it's transparent
			if (mapped_color >= 0x10) {
				continue;
			}

			this->set_pixel(points[i].x, points[i].y, mapped_color);
		}
	}
}

void Renderer::draw_oval(int x0, int y0, int x1, int y1, bool fill)
{
	int width = x1 - x0;
	int height = y1 - y0;

	if (width == 0 && height == 0) {
		return this->draw_point(x0, y0);
	}
	if (width == 0 || height == 0) {
		return this->draw_line(x0, y0, x1, y1);
	}

	// Apply camera
	Renderer_Point p0 = this->coord_to_screen(x0, y0);
	Renderer_Point p1 = this->coord_to_screen(x1, y1);
	x0 = p0.x;
	y0 = p0.y;
	x1 = p1.x;
	y1 = p1.y;

	/*
	* The formula for an ellipse is
	* (x/a)^2 + (y/b)^2 = 1
	* a = width/2, b = height/2, in math coordinates centered at point 0,0
	* 
	* The idea is to scan line-by-line, get all the x points that need to be drawn and.... draw them
	* for that, let's solve for x:
	* x = a * sqrt(1-(y/b)^2)
	* at y = 0, x[0] = a. If `fill`, then draw from x=-a to x=a. Otherwise, draw poitns x=-a and x=a
	* at y = 1 we can get a different x[1] <= x[0]. Draw all the points from x[0]-1 to x[1]
	* repeat until y = b
	* And translate all math coordinates to logical coordinates. Easy.
	*/


	double a = width / 2;
	double b = height / 2;

	// Center in screen coordinates
	double scx = (double)x0 + a;
	double scy = (double)y0 + b;

	double y = 0;
	double x = a;
	int sx = round(scx + x);
	int sy = round(scy);
	// The inverted point in integer part comes from the invariant (xf-x == x'-x0): The distance between the extremes and the points is constant
	// rearanging, we find x'=xf+x0-x
	int sxi = x1 + x0 - sx;
	int syi = y1 + y0 - sy;
	if (fill) {
		this->set_line(sxi, sx, sy);
		this->set_line(sxi, sx, syi);
	}
	else {
		this->set_point(sx, sy);
		this->set_point(sxi, sy);
		this->set_point(sx, syi);
		this->set_point(sxi, syi);
	}

	y += 1;
	while (y < b) {
		double tmp = y / b;
		double new_x = a * sqrt(1 - tmp * tmp);
		int new_sx = round(scx + new_x);
		sy = round(scy - y);
		sxi = x1 + x0 - new_sx;
		syi = y1 + y0 - sy;

		if (new_sx == sx) {
			// Simple, do the same as before.
			sx = new_sx;
			if (fill) {
				this->set_line(sxi, sx, sy);
				this->set_line(sxi, sx, syi);
			}
			else {
				this->set_point(sx, sy);
				this->set_point(sxi, sy);
				this->set_point(sx, syi);
				this->set_point(sxi, syi);
			}
		}
		else {
			/* We'll have to paint from x=new_x to x-1. Visualize this:
			[nx]     -> Covered by previous if condition
			[x ]

			[nx]     -> trivial case, only 1 pixel needs to be drawn
			    [x]

		    [nx][ ]  -> draw from nx to x-1
			       [x]
			*/
			int prev_sxi = x1 + x0 - sx;
			if (fill) {
				this->set_line(prev_sxi + 1, sx - 1, sy);
				this->set_line(prev_sxi + 1, sx - 1, syi);
			}
			else {
				this->set_line(new_sx, sx - 1, sy);
				this->set_line(prev_sxi+1, sxi, sy);
				this->set_line(new_sx, sx - 1, syi);
				this->set_line(prev_sxi + 1, sxi, syi);
			}
			sx = new_sx;
		}

		y += 1;
	}

	// The lid is trivial
	this->set_line(sxi + 1, sx - 1, y0);
	this->set_line(sxi + 1, sx - 1, y1);
}

void Renderer::draw_rectangle(int x0, int y0, int x1, int y1, bool fill)
{
	if (x0 > x1) {
		int t = x0;
		x0 = x1;
		x1 = t;
	}
	if (y0 > y1) {
		int t = y0;
		y0 = y1;
		y1 = t;
	}

	if (fill) {
		Renderer_Point sc_start = this->coord_to_screen(x0, y0);
		Renderer_Point sc_end = this->coord_to_screen(x1, y1);

		this->apply_clip(&sc_start.x, &sc_start.y, &sc_end.x, &sc_end.y);

		for (int y = sc_start.y; y <= sc_end.y; y++) {
			this->set_line(sc_start.x, sc_end.x, y);
		}
	}
	else {
		this->draw_line(x0, y0, x0, y1);
		this->draw_line(x0, y1, x1, y1);
		this->draw_line(x0, y0, x1, y0);
		this->draw_line(x1, y0, x1, y1);
	}
}

void Renderer::scroll(unsigned char lines)
{
	if (lines >= 128) {
		this->clear_screen(0);
	}
	else {
		unsigned int length = (128 - lines) * LINE_JMP;
		memmove(&p8_memory[ADDR_SCREEN], &p8_memory[ADDR_SCREEN + lines * LINE_JMP], length);
		memset(&p8_memory[ADDR_SCREEN + length], 0, 0x2000 - length);
	}
}

// Canvas cooridantes (without camera transform)
bool Renderer::will_be_drawn(int x, int y, int w, int h)
{
	int screen_x = x - (short)memory_read_short(ADDR_DS_CAMERA_X);
	int screen_y = y - (short)memory_read_short(ADDR_DS_CAMERA_Y);

	return is_drawable(screen_x, screen_y) || is_drawable(screen_x+w, screen_y) || is_drawable(screen_x, screen_y+h) ||is_drawable(screen_x+w, screen_y+h);
}

void Renderer::reset_draw_pal()
{
	char transp_mask = 0x10; // Color 0 = black set to transparent
	for (int i = 0; i < 16; i++) {
		p8_memory[ADDR_DS_DRAW_PAL + i] = i | transp_mask;
		transp_mask = 0; // Only the first mask
	}
}

void Renderer::reset_screen_pal()
{
	for (int i = 0; i < 16; i++) {
		p8_memory[ADDR_DS_SCREEN_PAL + i] = i;
	}
}

void Renderer::set_color_transparent(unsigned char color, bool transparent)
{
	if (transparent) {
		p8_memory[ADDR_DS_DRAW_PAL + color] = p8_memory[ADDR_DS_DRAW_PAL + color] | 0x10;
	} else {
		p8_memory[ADDR_DS_DRAW_PAL + color] = p8_memory[ADDR_DS_DRAW_PAL + color] & 0x0F;
	}
}

void Renderer::reset_transparency_pal()
{
	char transp_mask = 0x10; // Color 0 = black set to transparent
	for (int i = 0; i < 16; i++) {
		p8_memory[ADDR_DS_DRAW_PAL + i] = (p8_memory[ADDR_DS_DRAW_PAL + i] & 0x0F) | transp_mask;
		transp_mask = 0; // Only the first mask
	}
}

#include <map>
void Renderer::present()
{
	bool palette_changed = false;
	for (int i = 0; !palette_changed && i<16; i++) {
		palette_changed = this->prev_screen_pal[i] != p8_memory[ADDR_DS_SCREEN_PAL+i];
	}
	if (palette_changed) {
		memcpy(this->prev_screen_pal, &p8_memory[ADDR_DS_SCREEN_PAL], 16);
	}

	std::vector<SDL_Point> points[16];
	for (int i = 0; i < SCREEN_MEMORY_SIZE; i++) {
		unsigned char pixels = p8_memory[ADDR_SCREEN + i];
		unsigned char previous = this->prev_screen[i];
		if (!palette_changed && pixels == previous) {
			continue;
		}

		int x = (i % 64) * 2;
		int y = i / 64;

		if (palette_changed || ((pixels & 0x0F) != (previous & 0x0F))) {
			points[pixels & 0x0F].push_back(SDL_Point{ x,y });
		}
		if (palette_changed || ((pixels & 0xF0) != (previous & 0xF0))) {
			points[pixels >> 4].push_back(SDL_Point{ x+1,y });
		}
	}

	for (int i = 0; i < 16; i++) {
		int size = points[i].size();
		if (size == 0) {
			continue;
		}

		unsigned char paletteColor = p8_memory[ADDR_DS_SCREEN_PAL + i];
		SDL_Color color = DEFAULT_PALETTE[paletteColor & 0x0F];
		if (paletteColor >= 0x10) {
			color = EXTENDED_PALETTE[paletteColor & 0x0F];
		}
		SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, 0xFF);
		SDL_RenderDrawPoints(this->renderer, &points[i][0], size);
	}

	SDL_UpdateWindowSurface(this->window);
	memcpy(this->prev_screen, &p8_memory[ADDR_SCREEN], SCREEN_MEMORY_SIZE);

	timestamp_t now = getTimestamp();
	long long timediff = getMillisecondsDiff(now, this->prev_frame);
	if (timediff < 33) {
		this->sync_delay = timediff;
	}
	else {
		this->sync_delay = 0;
	}
	this->prev_frame = now;
}

// This is only called by flip - runningCart has a better control of FPS and might skip a render if needed.
void Renderer::syncrhonize_30fps()
{
	if (this->sync_delay > 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(this->sync_delay));
		this->sync_delay = 0;
	}
}

void Renderer::set_pixel(int sx, int sy, unsigned char color)
{
	color = color & 0x0F;
	int addr = ADDR_SCREEN + sy * LINE_JMP + sx / 2;
	if (sx % 2 == 0) {
		p8_memory[addr] = (p8_memory[addr] & 0xF0) | color;
	}
	else {
		p8_memory[addr] = (p8_memory[addr] & 0x0F) | (color << 4);
	}
}

unsigned char Renderer::get_pixel(int sx, int sy)
{
	int addr = ADDR_SCREEN + sy * LINE_JMP + sx / 2;
	if (sx % 2 == 0) {
		return p8_memory[addr] & 0x0F;
	}
	return p8_memory[addr] >> 4;
}

void Renderer::set_pixel_pair(int sx, int sy, unsigned char colors)
{
	int addr = ADDR_SCREEN + sy * LINE_JMP + sx / 2;
	p8_memory[addr] = colors;
}

void Renderer::set_line(int sx0, int sxf, int sy) {

	// Clip
	if (!this->is_y_drawable(sy)) return;
	sx0 = std::max(sx0, (int)p8_memory[ADDR_DS_CLIP_RECT]);
	sxf = std::min(sxf, (int)p8_memory[ADDR_DS_CLIP_RECT+2]);

	unsigned char color = p8_memory[ADDR_DS_COLOR];

	// Grab pattern for this line
	unsigned short pattern = memory_read_short(ADDR_DS_FILL_PAT);
	unsigned char bit = 12 - (sy % 4) * 4;
	unsigned short mask = 0x0F << bit;
	unsigned char value = (pattern & mask) >> bit;

	// Precalculate screen colors
	unsigned char colors[] = {
		(unsigned char)(this->get_screen_color(color & 0x0F) & 0x0F),
		(unsigned char)(this->get_screen_color((color & (0x0F << 4)) >> 4) & 0x0F)
	};

	// If there's transparency, we can't optimize using memset
	unsigned char off_is_transparent = (p8_memory[ADDR_DS_FILL_PAT + 2] & 0x80) > 0;
	if (off_is_transparent && value == 0x0F) {
		// Everything in this line is transparent lol
		return;
	}
	// We can exclude when value == 0, because then all the line is using a color. Also, skip optimization if less than 8 pixels must be drawn. This also excludes sx0 > sxf
	if ((off_is_transparent && value != 0) || (sxf - sx0) < 8) {
		for (int x = sx0; x <= sxf; x++) {
			unsigned char bit = 3 - x % 4;
			unsigned short mask = 0x1 << bit;
			unsigned char subvalue = (value & mask) >> bit;
			if (off_is_transparent && subvalue == 1) {
				continue;
			}
			this->set_pixel(x, sy, colors[subvalue]);
		}
		return;
	}

	// Now we can forget about transparency. The only way off_is_transparent can be true is if value == 0, which means that everything needs to be filled.

	// from sx0 to (x%4 == 0) we need to do it manually
	int x = sx0;
	while (x % 4 > 0) {
		unsigned char bit = 3 - x % 4;
		unsigned short mask = 0x1 << bit;
		unsigned char subvalue = (value & mask) >> bit;
		this->set_pixel(x, sy, colors[subvalue]);
		x++;
	}

	int start_offset = ADDR_SCREEN + sy * LINE_JMP + x / 2;
	// Draw 4 pixels
	for (int i = 0; i < 4; i++) {
		unsigned char bit = 3 - x % 4;
		unsigned short mask = 0x1 << bit;
		unsigned char subvalue = (value & mask) >> bit;
		this->set_pixel(x, sy, colors[subvalue]);
		x++;
	}

	int end_offset = ADDR_SCREEN + sy * LINE_JMP + (sxf & 0xFC) / 2;
	int bytes_available = 2;
	while (x < (sxf & 0xFC)) {
		int offset = ADDR_SCREEN + sy * LINE_JMP + x / 2;
		int l = std::min(end_offset - offset, bytes_available);
		memcpy(&p8_memory[offset], &p8_memory[start_offset], l);
		x += l*2;
		bytes_available += l;
	}

	// Draw the last pixels
	while (x <= sxf) {
		unsigned char bit = 3 - x % 4;
		unsigned short mask = 0x1 << bit;
		unsigned char subvalue = (value & mask) >> bit;
		this->set_pixel(x, sy, colors[subvalue]);
		x++;
	}
}

void Renderer::set_point(int sx, int sy) {
	if (!this->is_drawable(sx, sy)) return;

	unsigned char color = p8_memory[ADDR_DS_COLOR];
	unsigned char mapped_color = this->get_screen_pat_color(color, sx, sy);
	this->set_pixel(sx, sy, mapped_color);
}

Renderer_Point Renderer::coord_to_screen(int x, int y)
{
	int sx = x - (short)memory_read_short(ADDR_DS_CAMERA_X);
	int sy = y - (short)memory_read_short(ADDR_DS_CAMERA_Y);
	return Renderer_Point{ sx, sy };
}

bool Renderer::is_y_drawable(int sy) {
	unsigned char y0 = p8_memory[ADDR_DS_CLIP_RECT + 1];
	unsigned char y1 = p8_memory[ADDR_DS_CLIP_RECT + 3];
	return y0 <= sy && sy <= y1;
}
bool Renderer::is_x_drawable(int sx) {
	unsigned char x0 = p8_memory[ADDR_DS_CLIP_RECT];
	unsigned char x1 = p8_memory[ADDR_DS_CLIP_RECT + 2];
	return (x0 <= sx && sx <= x1);
}

bool Renderer::is_drawable(int sx, int sy)
{
	return this->is_x_drawable(sx) && this->is_y_drawable(sy);
}

void Renderer::apply_clip(int* sx0, int* sy0, int* sxf, int* syf)
{
	unsigned char cx0 = p8_memory[ADDR_DS_CLIP_RECT];
	unsigned char cy0 = p8_memory[ADDR_DS_CLIP_RECT + 1];
	unsigned char cxf = p8_memory[ADDR_DS_CLIP_RECT + 2];
	unsigned char cyf = p8_memory[ADDR_DS_CLIP_RECT + 3];

	*sx0 = std::max(*sx0, (int)cx0);
	*sy0 = std::max(*sy0, (int)cy0);
	*sxf = std::min(*sxf, (int)cxf);
	*syf = std::min(*syf, (int)cyf);
}

unsigned char Renderer::get_screen_color(unsigned char color)
{
	return p8_memory[ADDR_DS_DRAW_PAL + (color & 0x0F)];
}

unsigned char Renderer::get_screen_pat_color(unsigned char color, int sx, int sy)
{
	unsigned short pattern = memory_read_short(ADDR_DS_FILL_PAT);
	unsigned char off_is_transparent = (p8_memory[ADDR_DS_FILL_PAT + 2] & 0x80) > 0;

	unsigned char bit = 15 - ((sy % 4) * 4 + sx % 4);
	unsigned short mask = 0x1 << bit;
	unsigned short value = (pattern & mask) >> bit;

	if (off_is_transparent && value == 1) {
		return 0x10;
	}

	// 0 => 0x0F, 1 => 0xF0
	unsigned char bitshift = value * 4;

	color = (color & (0x0F << bitshift)) >> bitshift;
	return this->get_screen_color(color) & 0x0F; // It seems like all draw fns effected by pattern ignore palt
}
