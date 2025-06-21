#include "font.h"

#include <iostream>
#include <fstream>
#include <string>

#ifdef __PS4__
#define PATH "/app0/assets/misc/p8_font.txt"
#elif __SWITCH__
#define PATH "romfs:/misc/p8_font.txt"
#else
#define PATH "../assets/misc/p8_font.txt"
#endif

#include "log.h"
#include "memory.h"
#include "machine_state.h" // TODO circular depedency! => It's needed because control codes can change and persist color

CharData read_next(std::ifstream &file);

#define DEBUGLOG Font_DEBUGLOG
Log DEBUGLOG = logger.log("font");

unsigned char get_code_of_special_char(std::string specialChar);

Font::Font()
{
	std::ifstream myfile;
	myfile.open(PATH, std::ios::in);

	std::string line;
	while (std::getline(myfile, line)) {
		if (line != "") {
			CharData c = read_next(myfile);
			this->charData[get_code_of_special_char(line)] = c;
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

void Font::drawChar(unsigned char c, int x, int y)
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

	if (!renderer->will_be_drawn(x, y, charData.size, 5)) {
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

unsigned char read_param(char c) {
	if (c <= '9')
		return c - '0';
	else
		return 10 + c - 'a';
}
int Font::print(std::string c, int x, int y, bool scroll)
{
	int initial_x = x;
	int max_x = x;
	p8_memory[ADDR_DS_CURSOR_X] = x;
	p8_memory[ADDR_DS_CURSOR_HOME_X] = initial_x; // The cursor might overflow when using camera() - This is an issue on pico8 itself
	p8_memory[ADDR_DS_CURSOR_Y] = y;

	int start = 0;
	
	//we will need to update the cursor. This can change with modifiers such as \^w \n \^g \| \+
	int y_offset = 6;
	int y_original = y;
	int prev_width = 0; // Needed for backspace
	int bg_color = -1;
	int repeats = 1;

	while (start < c.length()) {
		max_x = std::max(x, max_x);
		unsigned char character = c[start];

		// Inline modifiers https://pico-8.fandom.com/wiki/P8SCII_Control_Codes
		if (character == 1) {
			repeats = read_param(c[start + 1]);
			start += 2;
			continue;
		}
		if (character == 2) {
			bg_color = read_param(c[start + 1]);
			start += 2;
			continue;
		}
		else if (character == 3) {
			int offset = read_param(c[start + 1]) - 16;
			x += offset;
			start += 2;
			continue;
		}
		else if (character == 4) {
			int offset = read_param(c[start + 1]) - 16;
			y += offset;
			start += 2;
			continue;
		}
		else if (character == 5) {
			int x_offset = read_param(c[start + 1]) - 16;
			int y_offset = read_param(c[start + 2]) - 16;
			x += x_offset;
			y += y_offset;
			start += 3;
			continue;
		}
		else if (character == 6) {
			char next = c[++start];
			if (next == 'c') {
				alert_todo("font: clear command");
				int color = read_param(c[start + 1]);
				renderer->clear_screen(color);
				x = 0; y = 0;
				start += 2;
				continue;
			}
			else if (next == 'g') {
				x = initial_x;
				y = y_original;
				start++;
				continue;
			}
			else if (next == 'h') {
				alert_todo("font: home command");
				initial_x = x;
				y_original = y;
				start++;
				continue;
			}
			else if (next == 'j') {
				x = read_param(c[start + 1]) * 4;
				y = read_param(c[start + 2]) * 4;
				start += 3;
				continue;
			}
			else if (next == '-') {
				next = c[++start];
				if (next == '#') {
					bg_color = -1;
					start++;
					continue;
				}
				else {
					alert_todo("font: unknown disable command <" + std::to_string(next) + ">");
				}
			}
			else {
				alert_todo("font: unknown command <" + std::to_string(next) + ">");
			}

			start++;
			continue;
		}
		else if (character == '\a') {
			// TODO Audio command
			start++;
			continue;
		}
		else if (character == '\b') {
			x -= prev_width;
			start++;
			continue;
		}
		else if (character == '\t') {
			// TODO Tab stop from memory 5f5a (Print attributes - 5f58+2)
			start++;
			continue;
		}
		else if (character == '\n') {
			x = initial_x;
			y += y_offset;
			p8_memory[ADDR_DS_CURSOR_Y] = y;
			y_offset = 6;
			start++;
			continue;
		}
		else if (character == '\v') {
			// TODO Decorate: Temporarily moves cursor to a spot and back to where it was (e.g. cafe\vb, puts a ` on e)
			start++;
			continue;
		}
		else if (character == '\f') { // Foreground color
			p8_memory[ADDR_DS_COLOR] = read_param(c[start + 1]);
			start+=2;
			continue;
		}
		else if (character == '\r') {
			x = p8_memory[ADDR_DS_CURSOR_HOME_X];
			start++;
			continue;
		}

		if (!this->charData.count(character)) {
			alert_todo("print: Couldn't find CharData for <" + std::to_string((unsigned char)character) + ">");
			start++;
			continue;
		}

		CharData c = this->charData[character];
		for (int i = 0; i < repeats; i++) {
			if (bg_color >= 0) {
				unsigned char original_color = p8_memory[ADDR_DS_COLOR];
				p8_memory[ADDR_DS_COLOR] = bg_color;
				renderer->draw_rectangle(x - 1, y - 1, x + c.size, y + 5, true);
				p8_memory[ADDR_DS_COLOR] = original_color;
			}
			this->drawChar(character, x, y);
			x += c.size + 1;
		}
		prev_width = c.size + 1;
		repeats = 1;

		start++;
	}

	p8_memory[ADDR_DS_CURSOR_X] = p8_memory[ADDR_DS_CURSOR_HOME_X];
	p8_memory[ADDR_DS_CURSOR_Y] += y_offset;
	if (scroll && ((p8_memory[ADDR_DS_CURSOR_Y]+6) >= 128)) {
		p8_memory[ADDR_DS_CURSOR_Y] -= y_offset;
		renderer->scroll(y_offset);
	}

	max_x = std::max(x, max_x);
	return max_x - initial_x;
}

unsigned char grapheme_to_code(std::string specialChar);
void Font::sys_print(std::string c, int x, int y)
{
	this->sys_print(c, x, y, 1);
}
void Font::sys_print(std::string c, int x, int y, double scale)
{
	int start = 0;

	while (start < c.length()) {
		int l = 1;
		unsigned char character = c[start];
		for (; l < 8 && (character == 0 || this->charData.count(character) == 0); l++) {
			std::string grapheme = c.substr(start, l);
			character = grapheme_to_code(grapheme);
		}

		// logger << ("orig: " + c + ", trying: " + grapheme).c_str();
		if (!this->charData.count(character)) {
			DEBUGLOG << "sys_print: Couldn't find CharData" << ENDL;
			start++;
			continue;
		}

		CharData charData = this->charData[character];

		if (charData.coords.size() == 0) {
			start++;
			x += (charData.size + 1) * SYS_SCALE * scale;
			continue;
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

// TODO https://www.lexaloffle.com/bbs/?tid=3739
#define SPECIAL_CHAR_OFFSET 0x7E
// Printable (with the font we have so far)
std::string special_chars[] = {
	"~", "○", "█", "▒", "🐱", "⬇️", "░", "✽", "●", "♥", "☉", "웃", "⌂", "⬅️",
	"😐", "♪", "🅾️", "◆", "…", "➡️", "★", "⧗", "⬆️", "ˇ", "∧", "❎", "▤",
	"▥"
};
/* Complete version
std::string special_chars[] = {
	"~", "○", "█", "▒", "🐱", "⬇️", "░", "✽", "●", "♥", "☉", "웃", "⌂", "⬅️",
	"😐", "♪", "🅾️", "◆", "…", "➡️", "★", "⧗", "⬆️", "ˇ", "∧", "❎", "▤",
	"▥", "あ", "い", "う", "え", "お", "か", "き", "く", "け", "こ", "さ", "し",
	"す", "せ", "そ", "た", "ち", "つ", "て", "と", "な", "に", "ぬ", "ね", "の",
	"は", "ひ", "ふ", "へ", "ほ", "ま", "み", "む", "め", "も", "や", "ゆ", "よ",
	"ら", "り", "る", "れ", "ろ", "わ", "を", "ん", "っ", "ゃ", "ゅ", "ょ", "ア",
	"イ", "ウ", "エ", "オ", "カ", "キ", "ク", "ケ", "コ", "サ", "シ", "ス", "セ",
	"ソ", "タ", "チ", "ツ", "テ", "ト", "ナ", "ニ", "ヌ", "ネ", "ノ", "ハ", "ヒ",
	"フ", "ヘ", "ホ", "マ", "ミ", "ム", "メ", "モ", "ヤ", "ユ", "ヨ", "ラ", "リ",
	"ル", "レ", "ロ", "ワ", "ヲ", "ン", "ッ", "ャ", "ュ", "ョ", "◜", "◝"
};
*/

unsigned char get_code_of_special_char(std::string specialChar) {
	if (specialChar.length() == 1) {
		return specialChar[0];
	}
	for (unsigned char c = 0; c < sizeof(special_chars) / sizeof(special_chars[0]); c++) {
		if (special_chars[c] == specialChar) {
			return SPECIAL_CHAR_OFFSET + c;
		}
	}
	DEBUGLOG << "Can't find code for character " << specialChar << " when loading font file" << ENDL;
	return '?';
}

unsigned char grapheme_to_code(std::string specialChar) {
	for (unsigned char c = 0; c < sizeof(special_chars) / sizeof(special_chars[0]); c++) {
		if (special_chars[c] == specialChar) {
			return SPECIAL_CHAR_OFFSET + c;
		}
	}
	return 0;
}
