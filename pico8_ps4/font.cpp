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

#define DEBUGLOG Font_DEBUGLOG
Log DEBUGLOG = logger.log("font");

Font::Font()
{
	std::ifstream myfile;
	myfile.open(PATH, std::ios::in);

	std::string line;
	while (std::getline(myfile, line)) {
		if (line != "") {
			CharData c = read_next(myfile);
			this->charData[line] = c;
		}
	}

	myfile.close();
}

void Font::initialize()
{
	p8_memory[ADDR_DS_CURSOR_HOME_X] = 0;
	p8_memory[ADDR_DS_CURSOR_X] = 0;
	p8_memory[ADDR_DS_CURSOR_Y] = 0;
}

void Font::drawChar(std::string c, int x, int y)
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

	std::vector<Renderer_Point> newCoords(charData.coords.size());
	for (int i = 0; i < charData.coords.size(); i++) {
		Renderer_Point original = charData.coords[i];

		newCoords[i].x = x + original.x;
		newCoords[i].y = y + original.y;
	}
	renderer->draw_points(newCoords);
}

void Font::print(std::string c, int x, int y, bool scroll)
{
	p8_memory[ADDR_DS_CURSOR_X] = x;
	p8_memory[ADDR_DS_CURSOR_Y] = y;

	int start = 0;
	
	//we will need to update the cursor. This can change with modifiers such as \^w \n \^g \| \+
	int y_offset = 6;
	int x_original = x;

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
		else if (grapheme == "\n") {
			x = x_original;
			y += y_offset;
			y_offset = 6;
			start += l;
			continue;
		}

		// logger << ("orig: " + c + ", trying: " + grapheme).c_str();
		while (!this->charData.count(grapheme) && l < 8) {
			l++;
			grapheme = c.substr(start, l);
		}
		if (!this->charData.count(grapheme)) {
			DEBUGLOG << "print: Couldn't find CharData for " << grapheme << ENDL;
			continue;
		}

		CharData c = this->charData[grapheme];
		this->drawChar(grapheme, x, y);
		x += c.size + 1;
		start += l;
	}

	p8_memory[ADDR_DS_CURSOR_Y] += y_offset;
	if (scroll && ((p8_memory[ADDR_DS_CURSOR_Y]+6) >= 128)) {
		p8_memory[ADDR_DS_CURSOR_Y] -= y_offset;
		renderer->scroll(y_offset);
	}
}

void Font::sys_print(std::string c, int x, int y)
{
	this->sys_print(c, x, y, 1);
}
void Font::sys_print(std::string c, int x, int y, double scale)
{
	int start = 0;

	while (start < c.length()) {
		int l = 1;
		std::string grapheme = c.substr(start, l);
		// logger << ("orig: " + c + ", trying: " + grapheme).c_str();
		while (!this->charData.count(grapheme) && l < 8) {
			l++;
			grapheme = c.substr(start, l);
		}
		if (!this->charData.count(grapheme)) {
			DEBUGLOG << "sys_print: Couldn't find CharData for " << grapheme << ENDL;
			continue;
		}

		CharData charData = this->charData[grapheme];

		if (charData.coords.size() == 0) {
			return;
		}

		for (int i = 0; i < charData.coords.size(); i++) {
			SDL_Rect r{
				(int)(x + (charData.coords[i].x * SYS_SCALE * scale)),
				(int)(y + (charData.coords[i].y * SYS_SCALE * scale)),
				(int)(SYS_SCALE * scale),
				(int)(SYS_SCALE * scale)
			};
			SDL_RenderFillRect(renderer->renderer, &r);
		}

		x += (charData.size + 1) * SYS_SCALE * scale;
		start += l;
	}
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
				Renderer_Point c{
					x, y
				};
				ret.coords.push_back(c);
			}
		}

		y++;
	}

	return ret;
}
