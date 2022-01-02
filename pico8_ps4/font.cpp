#include "font.h"

#include <iostream>
#include <fstream>
#include <string>

#ifdef __PS4__
#define PATH "/app0/assets/misc/p8_font.txt"
#else
#define PATH "../assets/misc/p8_font.txt"
#endif

#include "log.h"
#include "memory.h"
#include "machine_state.h" // TODO circular depedency! => It's needed because control codes can change and persist color

CharData read_next(std::ifstream &file);

Font::Font()
{
	std::ifstream myfile;
	myfile.open(PATH, std::ios::in);

	std::string line;
	while (std::getline(myfile, line)) {
		if (line != "") {
			CharData c = read_next(myfile);
			this->charData[line] = c;
			// logger << (line + " inserted").c_str();
		}
	}

	myfile.close();
}

void Font::drawChar(std::string c, int x, int y, SDL_Renderer* renderer)
{
	if (!this->charData.count(c)) {
		logger << "Font::drawChar: Couldn't find CharData for " << c << ENDL;
		return;
	}

	CharData charData = this->charData[c];
	// logger << c.c_str() << " " << c.length() << " " << charData.coords.size() << ", " << x << " " << y << ENDL;

	if (charData.coords.size() == 0) {
		// Space :)
		return;
	}

	std::vector<SDL_Point> newCoords(charData.coords.size());
	for (int i = 0; i < charData.coords.size(); i++) {
		SDL_Point original = charData.coords[i];

		newCoords[i].x = x + original.x;
		newCoords[i].y = y + original.y;
	}
	SDL_RenderDrawPoints(renderer, &newCoords[0], newCoords.size());

	// I think this could be faster
	//SDL_Rect viewport{};
	//SDL_RenderGetViewport(renderer, &viewport);
	//viewport.x += x;
	//viewport.y += y;
	//SDL_RenderSetViewport(renderer, &viewport);
	//SDL_RenderDrawPoints(renderer, &charData.coords[0], charData.coords.size());
	//viewport.x -= x;
	//viewport.y -= y;
	//SDL_RenderSetViewport(renderer, &viewport);
}

int Font::print(std::string c, int x, int y, SDL_Renderer* renderer)
{
	int start = 0;
	
	// machine_state will need to know where the next line begins. This can change with modifiers such as \^w \n \^g \| \+
	int y_offset = 6;

	while (start < c.length()) {
		int l = 1;
		std::string grapheme = c.substr(start, l);

		// Inline modifiers https://pico-8.fandom.com/wiki/P8SCII_Control_Codes
		if (grapheme == "\\") {
			if (c[start + 1] == '-') {
				char offset_c = c[start + 2];
				int offset = 0;
				if (offset_c <= '9') {
					offset = (offset_c - '0') - 16;
				}
				else {
					offset = 10 + (offset_c - 'a') - 16;
				}
				x += offset;
				start += 3;
				continue;
			}
		}
		else if (grapheme == "\f") { // Foreground color
			char color_c = c[start + 1];
			int color;
			if (color_c <= '9')
				color = color_c - '0';
			else
				color = 10 + color_c - 'a';

			p8_memory[ADDR_DS_COLOR] = color;
			start += 2;
			continue;
		}

		// logger << ("orig: " + c + ", trying: " + grapheme).c_str();
		while (!this->charData.count(grapheme) && l < 8) {
			l++;
			grapheme = c.substr(start, l);
		}
		if (!this->charData.count(grapheme)) {
			logger << "Font::print: Couldn't find CharData for " << grapheme << ENDL;
			return y_offset;
		}


		CharData c = this->charData[grapheme];
		this->drawChar(grapheme, x, y, renderer);
		x += c.size + 1;
		start += l;
	}

	return y_offset;
}

CharData read_next(std::ifstream& file) {
	CharData ret;

	std::string line;
	int y = 0;
	ret.size = 3;
	while (std::getline(file, line)) {
		if (line == "") {
			return ret;
		}
		if (line.length() > ret.size) {
			ret.size = 7;
		}

		for (int x = 0; x < line.length(); x++) {
			if (line[x] == '#') {
				SDL_Point c{
					x, y
				};
				ret.coords.push_back(c);
			}
		}

		y++;
	}

	return ret;
}
