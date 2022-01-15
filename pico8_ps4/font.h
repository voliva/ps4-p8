#pragma once

#include <vector>
#include <map>
#include <string>
#include "renderer.h"

#define SYS_SCALE 10
#define SYS_CHAR_WIDTH SYS_SCALE * 4
#define SYS_CHAR_HEIGHT SYS_SCALE * 6

typedef struct {
	int size;
	std::vector<Renderer_Point> coords;
} CharData;

class Font
{
public:
	Font();
	void initialize();

	void drawChar(unsigned char c, int x, int y);
	void print(std::string c, int x, int y, bool scroll);

	void sys_print(std::string c, int x, int y);
	void sys_print(std::string c, int x, int y, double scale);

private:
	std::map<unsigned char, CharData> charData;
};

extern Font* font;
