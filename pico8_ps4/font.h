#pragma once

#include <vector>
#include <map>
#include <string>

typedef struct {
	int x;
	int y;
} Coord;

typedef struct {
	int size;
	std::vector<Coord> coords;
} CharData;

class Font
{
public:
	Font();

private:
	std::map<std::string, CharData> charData;
};

