#pragma once
#include <SDL2/SDL_render.h>
#include <vector>
#include <string>

typedef struct {
	int row;
	int col;
	std::string title;
	std::string author;
	std::string lid;
	std::string pid;
} SploreCartridge;

typedef struct {
	SDL_Surface* surface;
	std::vector<SploreCartridge> cartridges;
} SploreResult;

SploreResult* splore_get_featured();