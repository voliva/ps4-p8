#include "splore_loader.h"
#include "http.h"
#include <stb/stb_image.h>
#include "log.h"

#define DEBUGLOG SploreLoader_DEBUGLOG
Log DEBUGLOG = logger.log("SploreLoader");

/*
http://www.lexaloffle.com/bbs/get_cart.php?cat=7&play_src=2&lid=harold-4
=> https://www.lexaloffle.com/bbs/get_cart.php?cat=7&play_src=2&lid=harold-4
*/

std::string read_png_line(unsigned char* data, int width, int x, int line) {
	std::string result = "";
	char c;
	while ((c = (char)data[(line * width + x)*4]) != 0) {
		result += c;
		x++;
	}
	return result;
}

SploreResult* splore_get_featured()
{
    int width, height, channels;

    std::vector<unsigned char> png_data = http_get("http://www.lexaloffle.com/bbs/cpost_lister3.php?max=32&start_index=0&cat=7&sub=2&orderby=rating&version=000204w&cfil=0");
    unsigned char* data = stbi_load_from_memory(&png_data[0], png_data.size(), &width, &height, &channels, STBI_rgb_alpha);
    // unsigned char* data = stbi_load("../p8-cartridges/cpost_lister3.png", &width, &height, &channels, STBI_rgb_alpha);

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(data, width, height, 4 * 8, width * 4, 0x0000000FF, 0x00000FF00, 0x000FF0000, 0x0FF000000);

	std::vector<SploreCartridge> cartridges;
	int columns = width / 128;
	int rows = height / 136;
	for (int row = 0; row < rows; row++) {
		int line = row * 136;
		for (int col = 0; col < columns; col++) {
			int x = col * 128;

			std::string title, author, lid, pid;
			pid = read_png_line(data, width, x, line + 128);
			pid = pid.substr(pid.find_first_of(' ') + 1);
			pid = pid.substr(0, pid.find_first_of(' '));

			title = read_png_line(data, width, x, line + 129);
			author = read_png_line(data, width, x, line + 130);
			lid = read_png_line(data, width, x, line + 131);

			SploreCartridge info{ row, col, title, author, lid, pid };
			cartridges.push_back(info);
		}
	}

	return new SploreResult {
		surface,
		cartridges
	};
}
