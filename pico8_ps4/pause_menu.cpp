#include "pause_menu.h"
#include "renderer.h"
#include "memory.h"
#include "font.h"

void PauseMenu::initialize()
{
	this->active_index = 0;
	this->pressed = false;
}

int PauseMenu::manageEvent(KeyEvent& e)
{
	if (!e.down) {
		return 0;
	}

	switch (e.key) {
	case P8_Key::down:
		this->active_index = (this->active_index + 1) % 2;
		break;
	case P8_Key::up:
		this->active_index--;
		if (this->active_index < 0) {
			this->active_index = 1;
		}
		break;
	case P8_Key::pause:
		return 1;
	case P8_Key::circle:
	case P8_Key::cross:
		switch (this->active_index) {
		case 0: // continue
			return 1;
		case 1:
			return 2;
		}
		break;
	}
	return 0;
}

void PauseMenu::draw()
{
	unsigned char original_color = p8_memory[ADDR_DS_COLOR];

	int lines = 2;
	int height = 5 + lines * 8 - 3 + 5;
	int width = 2 * P8_WIDTH / 3;

	int left = (P8_WIDTH - width) / 2;
	int right = left + width;
	int top = (P8_HEIGHT - height) / 2;
	int bottom = top + height;

	p8_memory[ADDR_DS_COLOR] = 0;
	renderer->draw_rectangle(left, top, right, bottom, true);
	p8_memory[ADDR_DS_COLOR] = 7;
	renderer->draw_rectangle(left + 2, top + 2, right - 2, bottom - 2, false);

	for (int l = 0; l < lines; l++) {
		int text_top = top + 5 + l * 8;
		int text_x = left + 9;
		if (this->active_index == l) {
			// Arrow
			renderer->draw_line(left + 5, text_top, left + 5, text_top + 4);
			renderer->draw_line(left + 6, text_top+1, left + 6, text_top + 3);
			renderer->draw_point(left + 7, text_top+2);
			text_x += 1;
		}
		switch (l) {
		case 0:
			font->print("continue", text_x, text_top, false);
			break;
		/*case 1:
			font->print("reset", text_x, text_top, false);
			break;*/
		case 1:
			font->print("quit", text_x, text_top, false);
			break;
		}
	}

	renderer->present();
	p8_memory[ADDR_DS_COLOR] = original_color;
}

