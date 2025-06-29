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

#define SCALE 2.5
#define CART_SIZE 128

bool invalidateLocalCartridges = false;
void Splore::initialize(Mode m)
{
	if (this->texture != NULL) {
		SDL_DestroyTexture(this->texture);
		this->texture = NULL;
	}
	this->cartridges.clear();

	SploreResult* result;
	if (m == Mode::Featured)
		result = splore_get_featured();
	else
		result = splore_get_new();

	if (result == NULL) {
		this->carousel = NULL;
		DEBUGLOG << "SploreLoader didn't return a result" << ENDL; // TODO show to the user
		return;
	}

	this->texture = SDL_CreateTextureFromSurface(renderer->renderer, result->surface);
	this->cartridges = result->cartridges;
	SDL_FreeSurface(result->surface);

	this->carousel = new Carousel(this->cartridges.size(), CART_SIZE * SCALE, CART_SIZE * SCALE);

	delete result;
}

void Splore::key_down(Key k)
{
	if (this->carousel != NULL) {
		this->carousel->keyDown(k);
	}

	switch (k) {
	case Key::cross:
	{
		if (this->carousel == NULL) break;
		auto lid = this->cartridges[this->carousel->getActiveIndex()].lid;
		Cartridge* r = load_from_url("https://www.lexaloffle.com/bbs/get_cart.php?cat=7&play_src=2&lid=" + lid);
		run_cartridge(r, lid);
		delete r;
		break;
	}
	case Key::pause: {
		auto lid = this->cartridges[this->carousel->getActiveIndex()].lid;
		std::vector<MenuItem> items = {
			MenuItem {
				"Save cartridge",
				[lid]() {
					std::vector<unsigned char> png_data = http_get("https://www.lexaloffle.com/bbs/get_cart.php?cat=7&play_src=2&lid=" + lid);
					std::string filename = (std::string)CARTRIDGE_FOLDER + "/" + lid + ".p8.png";
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

void Splore::render(long long delta)
{
	if (!this->texture || !this->carousel) {
		return;
	}


	auto items = carousel->draw(delta);

	for (int i = 0; i < items.size(); i++) {
		auto item = items[i];
		if (item.idx < 0 || item.idx >= this->cartridges.size()) continue;

		SDL_Rect src{
			this->cartridges[item.idx].col * 128,
			this->cartridges[item.idx].row * 136,
			128,
			128
		};
		SDL_SetTextureAlphaMod(this->texture, item.alpha);
		SDL_RenderCopy(renderer->renderer, this->texture, &src, &item.destRect);
	}

	SploreCartridge focused = this->cartridges[this->carousel->getActiveIndex()];

	font->sys_print(
		focused.title,
		(FRAME_WIDTH - SYS_CHAR_WIDTH * focused.title.length()) / 2,
		30 + SYS_CHAR_HEIGHT + 100 + CART_SIZE * SCALE + 80
	);
	std::string author = "by: " + focused.author;
	font->sys_print(
		author,
		(FRAME_WIDTH - SYS_CHAR_WIDTH * author.length()) / 2,
		30 + SYS_CHAR_HEIGHT + 100 + CART_SIZE * SCALE + 80 + SYS_CHAR_HEIGHT + 10
	);
}
