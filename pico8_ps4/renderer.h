#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include "chrono.h"

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

#define SCREEN_MEMORY_SIZE P8_HEIGHT * P8_WIDTH / 2

typedef struct {
	int x;
	int y;
} Renderer_Point;

class Renderer {
public:
	Renderer();
	void initialize();

	void clear_screen(unsigned char color);
	void draw_map(int cx, int cy, int sx, int sy, int cw, int ch, unsigned char layer);
	void draw_sprite(int n, int x, int y, int w, int h, bool flip_x, bool flip_y);
	void draw_from_spritesheet(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, bool flip_x, bool flip_y);
	void draw_point(int x, int y);
	void draw_points(std::vector<Renderer_Point> &points);
	void draw_line(int x0, int y0, int x1, int y1);
	void draw_textured_line(int x0, int y0, int x1, int y1, float mx, float my, float mdx, float mdy);
	void draw_oval(int x0, int y0, int x1, int y1, bool fill);
	void draw_rectangle(int x0, int y0, int x1, int y1, bool fill);
	void scroll(unsigned char lines);
	bool will_be_drawn(int x, int y, int w, int h);
	unsigned char get_pixel(int x, int y);

	// Palette
	void reset_draw_pal();
	void reset_screen_pal();
	void set_color_transparent(unsigned char color, bool transparent);
	void reset_transparency_pal();

	void present();
	void syncrhonize_30fps();

	SDL_Renderer* renderer;
	SDL_Window* window;

private:
	void set_pixel(int sx, int sy, unsigned char color);
	void set_pixel_pair(int sx, int sy, unsigned char colors);
	// Same but use the draw state
	void set_line(int sx0, int sxf, int sy);
	void set_point(int sx, int sy);

	// Camera
	Renderer_Point coord_to_screen(int x, int y);

	// Clip
	bool is_x_drawable(int sx);
	bool is_y_drawable(int sy);
	bool is_drawable(int sx, int sy);
	void apply_clip(int* sx0, int* sy0, int* sxf, int* syf);

	// Screen color
	unsigned char get_screen_color(unsigned char color);
	unsigned char get_screen_pat_color(unsigned char color, int sx, int sy);

	unsigned char prev_screen[SCREEN_MEMORY_SIZE];
	unsigned char prev_screen_pal[16];

	timestamp_t prev_frame;
	int sync_delay;
};
extern Renderer *renderer;