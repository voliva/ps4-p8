#pragma once

#include <vector>
#include <map>
#include <string>
#include <SDL2/SDL.h>

typedef struct {
	int size;
	std::vector<SDL_Point> coords;
} CharData;

class Font
{
public:
	Font();
	void drawChar(std::string c, int x, int y, SDL_Renderer *renderer);
	void print(std::string c, int x, int y, SDL_Renderer* renderer);

private:
	std::map<std::string, CharData> charData;
};

