#include "splore.h"
#include "log.h"
#include "font.h"
#include "renderer.h"
#include "cartridge.h"
#include "running-cart.h"
#include "system_menu.h"
#include "http.h"
#include "file_paths.h"

#define DEBUGLOG Splore_DEBUGLOG
Log DEBUGLOG = logger.log("Splore");

bool invalidateLocalCartridges = false;
void Splore::initialize(Mode m)
{
	if (this->texture != NULL) {
		SDL_DestroyTexture(this->texture);
		this->texture = NULL;
	}
	this->cartridges.clear();
	this->focus = 0;

	SploreResult* result;
	if (m == Mode::Featured)
		result = splore_get_featured();
	else
		result = splore_get_new();

	if (result == NULL) {
		DEBUGLOG << "SploreLoader didn't return a result" << ENDL; // TODO show to the user
		return;
	}

	this->texture = SDL_CreateTextureFromSurface(renderer->renderer, result->surface);
	this->cartridges = result->cartridges;
	SDL_FreeSurface(result->surface);

	delete result;
}

void Splore::key_down(Key k)
{
	switch (k) {
	case Key::left:
		if (this->focus == 0) break;
		this->focus--;
		break;
	case Key::right:
		if (this->focus == this->cartridges.size() - 1) break;
		this->focus++;
		break;
	case Key::cross:
	{
		Cartridge* r = load_from_url("https://www.lexaloffle.com/bbs/get_cart.php?cat=7&play_src=2&lid=" + this->cartridges[this->focus].lid);
		run_cartridge(r, this->cartridges[this->focus].lid);
		delete r;
		break;
	}
	case Key::pause: {
		std::vector<MenuItem> items = {
			MenuItem {
				"Save cartridge",
				[this]() {
					std::vector<unsigned char> png_data = http_get("https://www.lexaloffle.com/bbs/get_cart.php?cat=7&play_src=2&lid=" + this->cartridges[this->focus].lid);
					std::string filename = (std::string)CARTRIDGE_FOLDER + "/" + this->cartridges[this->focus].lid + ".p8.png";
					FILE *file = fopen(filename.c_str(), "wb");
					if (!file) {
						return;
					}
					fwrite(&png_data[0], 1, png_data.size(), file);
					fclose(file);
					invalidateLocalCartridges = true;
				}
			}
		};
		activeSystemMenu = new SystemMenu(items);
		break;
	}
	}
}

#define SCALE 2.5
void Splore::render()
{
	if (!this->texture) {
		return;
	}
	int width = 128;
	int height = 128;

	int x_center = FRAME_WIDTH / 2;
	double x = x_center - width * SCALE / 2;
	double y = 170;

	SDL_SetTextureAlphaMod(this->texture, 64);
	if (this->focus > 0) {
		SDL_Rect src{
			this->cartridges[this->focus-1].col * 128,
			this->cartridges[this->focus-1].row * 136,
			width,
			height
		};
		SDL_Rect dest{
			(int)(x_center - width * SCALE * 0.9 / 2 - width * SCALE * 0.5),
			(int)y + 20,
			(int)(width * SCALE * 0.9),
			(int)(height * SCALE * 0.9)
		};
		SDL_RenderCopy(renderer->renderer, this->texture, &src, &dest);
	}
	if (this->focus < this->cartridges.size() - 1) {
		SDL_Rect src{
			this->cartridges[this->focus + 1].col * 128,
			this->cartridges[this->focus + 1].row * 136,
			width,
			height
		};
		SDL_Rect dest{
			(int)(x_center - width * SCALE * 0.9 / 2 + width * SCALE * 0.5),
			(int)y + 20,
			(int)(width * SCALE * 0.9),
			(int)(height * SCALE * 0.9)
		};
		SDL_RenderCopy(renderer->renderer, this->texture, &src, &dest);
	}

	SDL_SetTextureAlphaMod(this->texture, 255);
	SploreCartridge focused = this->cartridges[this->focus];
	SDL_Rect src{
		focused.col * 128,
		focused.row * 136,
		width,
		height
	};
	SDL_Rect dest{
		(int)x,
		(int)y,
		(int)(width*SCALE),
		(int)(height*SCALE)
	};
	SDL_RenderCopy(renderer->renderer, this->texture, &src, &dest);

	font->sys_print(
		focused.title,
		(FRAME_WIDTH - SYS_CHAR_WIDTH * focused.title.length()) / 2,
		y + height*SCALE + 80
	);
	std::string author = "by: " + focused.author;
	font->sys_print(
		author,
		(FRAME_WIDTH - SYS_CHAR_WIDTH * author.length()) / 2,
		y + height * SCALE + 80 + SYS_CHAR_HEIGHT + 10
	);
}
