#pragma once

#include <SDL2/SDL.h>
#include <vector>

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

typedef struct {
	int x;
	int y;
} Renderer_Point;

class Renderer {
public:
	Renderer();
	void initialize();

	void clear_screen(unsigned char color);
	void draw_sprite(int n, int x, int y, int w, int h); // TODO flip
	void draw_from_spritesheet(int sx, int sy, int sw, int sh, int dx, int dy); // TODO stretch + flip
	void draw_point(int x, int y);
	void draw_points(std::vector<Renderer_Point> &points);
	void draw_line(int x0, int y0, int x1, int y1);
	void draw_circle(int x, int y, int radius, bool fill);
	void draw_rectangle(int x0, int y0, int x1, int y1, bool fill);
	void scroll(unsigned char lines);

	// Palette
	void reset_draw_pal();
	void reset_screen_pal();
	void set_color_transparent(unsigned char color, bool transparent);
	void reset_transparency_pal();

	void present();

private:
	SDL_Window* window;
	SDL_Renderer* renderer;

	void set_transform_pixel(int x, int y, unsigned char color, bool transparency);
};
extern Renderer *renderer;