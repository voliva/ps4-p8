#include "system_menu.h"
#include "renderer.h"
#include "font.h"

SystemMenu* activeSystemMenu = NULL;

SystemMenu::SystemMenu(std::vector<MenuItem>& items) {
	this->active_index = 0;
	this->pressed = false;
	this->items = items;
	this->items.push_back(MenuItem{
		"Cancel",
		[]() {
		}
	});
}

void SystemMenu::keyDown(Key key)
{
	switch (key) {
	case Key::down:
		this->active_index = (this->active_index + 1) % this->items.size();
		break;
	case Key::up:
		this->active_index--;
		if (this->active_index == 0xFF) {
			this->active_index = this->items.size() - 1;
		}
		break;
	case Key::pause:
		delete activeSystemMenu;
		activeSystemMenu = NULL;
		break;
	case Key::circle:
	case Key::cross:
		this->items[this->active_index].callback();
		delete activeSystemMenu;
		activeSystemMenu = NULL;
		break;
	}
}

std::string flipCase(const std::string& input);

void SystemMenu::draw()
{
	size_t maxLength = 0;
	for (int i = 0; i < this->items.size(); i++) {
		size_t len = this->items[i].label.length();
		if (len > maxLength) {
			maxLength = len;
		}
	}

	float scale = 0.5;
	int margins = 10;
	int border = 5;
	int charHeight = SYS_CHAR_HEIGHT * scale;
	int menuItemHeight = charHeight + margins * 2;
	int menuHeight = menuItemHeight * this->items.size();
	int menuWidth = SYS_CHAR_WIDTH * maxLength * scale + margins * 2;

	SDL_Rect menuRect = {
		(FRAME_WIDTH - menuWidth) / 2 - border,
		(FRAME_HEIGHT - menuHeight) / 2 - border,
		menuWidth + border * 2,
		menuHeight + border * 2
	};

	SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
	SDL_RenderFillRect(renderer->renderer, &menuRect);

	for (int i = 0; i < this->items.size(); ++i) {
		SDL_Color textColor = { 255, 255, 255, 255 };
		SDL_Color highlightColor = { 0, 128, 255, 255 };

		SDL_Rect itemRect = {
			menuRect.x + border,
			menuRect.y + i * menuItemHeight + border,
			menuWidth,
			menuItemHeight
		};

		// Highlight background if active
		if (i == active_index) {
			SDL_SetRenderDrawColor(renderer->renderer, highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);
			SDL_RenderFillRect(renderer->renderer, &itemRect);
		}

		SDL_SetRenderDrawColor(renderer->renderer, textColor.r, textColor.g, textColor.b, textColor.a);
		std::string label = flipCase(this->items[i].label);
		font->sys_print(
			label,
			itemRect.x + margins,
			itemRect.y + margins,
			scale
		);

		// Draw border
		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(renderer->renderer, &itemRect);
	}
}


std::string flipCase(const std::string& input)
{
	std::string result = input;
	for (char& c : result) {
		if (std::islower(static_cast<unsigned char>(c))) {
			c = std::toupper(static_cast<unsigned char>(c));
		}
		else if (std::isupper(static_cast<unsigned char>(c))) {
			c = std::tolower(static_cast<unsigned char>(c));
		}
	}
	return result;
}