#pragma once

#include <vector>
#include <string>

typedef struct {
	std::vector<unsigned char> sprite_map;
	std::vector<unsigned char> sprite_flags;
	std::vector<unsigned char> music;
	std::vector<unsigned char> sfx;
	std::string lua;
} Cartridge;

Cartridge *load_from_png(std::string path);
