#pragma once

#include <vector>
#include <map>
#include <string>
#include "renderer.h"

typedef struct {
	int size;
	std::vector<Renderer_Point> coords;
} CharData;

class Font
{
public:
	Font();
	void initialize();

	void drawChar(std::string c, int x, int y);
	void print(std::string c, int x, int y, bool scroll);

private:
	std::map<std::string, CharData> charData;
};

extern Font* font;
