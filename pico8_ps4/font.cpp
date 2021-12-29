#include "font.h"

#include <iostream>
#include <fstream>
#include <string>

#ifdef __PS4__
#define PATH "/app0/assets/misc/p8_font.txt"
#else
#define PATH "../assets/misc/p8_font.txt"
#endif

#include "log.h";

CharData read_next(std::ifstream &file);
extern Logger logger;

Font::Font()
{
	std::ifstream myfile;
	myfile.open(PATH, std::ios::in);

	std::string line;
	while (std::getline(myfile, line)) {
		if (line != "") {
			CharData c = read_next(myfile);
			this->charData[line] = c;
			logger << (line + " inserted").c_str();
		}
	}

	myfile.close();
}

CharData read_next(std::ifstream& file) {
	CharData ret;

	std::string line;
	int y = 0;
	int size = 3;
	while (std::getline(file, line)) {
		if (line == "") {
			return ret;
		}
		if (line.length() > size) {
			size = 7;
		}

		for (int x = 0; x < line.length(); x++) {
			if (line[x] == '#') {
				Coord c{
					x, y
				};
				ret.coords.push_back(c);
			}
		}

		y++;
	}

	return ret;
}
