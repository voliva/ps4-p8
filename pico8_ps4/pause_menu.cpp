#include "pause_menu.h"
#include "renderer.h"
#include "memory.h"
#include "font.h"
#include "running-cart.h"
#include "lua_state.h"
#include "machine_state.h"

#define DEFAULT_LINES 5

void PauseMenu::initialize()
{
	this->active_index = 0;
	this->pressed = false;
}

void PauseMenu::manageEvent(KeyEvent& e)
{
	if (!e.down) {
		return;
	}

	switch (e.key) {
	case P8_Key::down:
		this->active_index = (this->active_index + 1) % DEFAULT_LINES;
		break;
	case P8_Key::up:
		this->active_index--;
		if (this->active_index == 0xFF) {
			this->active_index = DEFAULT_LINES-1;
		}
		break;
	case P8_Key::pause:
		this->active_index = 0;
		runningCart->resume();
		break;
	case P8_Key::circle:
	case P8_Key::cross:
		switch (this->active_index) {
		case 0: // continue
			runningCart->resume();
			break;
		case 1:
			runningCart->restart();
			break;
		case 2:
			machineState->saveState();
			runningCart->resume();
			break;
		case 3:
			machineState->loadState();
			runningCart->resume();
			break;
		case 4:
			runningCart->stop();
			break;
		}
		this->active_index = 0;
		break;
	}
}

void PauseMenu::draw()
{
	unsigned char original_color = p8_memory[ADDR_DS_COLOR];
	unsigned short original_camera_x = memory_read_short(ADDR_DS_CAMERA_X);
	unsigned short original_camera_y = memory_read_short(ADDR_DS_CAMERA_Y);
	memory_write_short(ADDR_DS_CAMERA_X, 0);
	memory_write_short(ADDR_DS_CAMERA_Y, 0);

	int height = 5 + DEFAULT_LINES * 8 - 3 + 5;
	int width = 2 * P8_WIDTH / 3;

	int left = (P8_WIDTH - width) / 2;
	int right = left + width;
	int top = (P8_HEIGHT - height) / 2;
	int bottom = top + height;

	p8_memory[ADDR_DS_COLOR] = 0;
	renderer->draw_rectangle(left, top, right, bottom, true);
	p8_memory[ADDR_DS_COLOR] = 7;
	renderer->draw_rectangle(left + 2, top + 2, right - 2, bottom - 2, false);

	for (int l = 0; l < DEFAULT_LINES; l++) {
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
		case 1:
			font->print("reset cart", text_x, text_top, false);
			break;
		case 2:
			font->print("save state", text_x, text_top, false);
			break;
		case 3:
			font->print("load state", text_x, text_top, false);
			break;
		case 4:
			font->print("quit", text_x, text_top, false);
			break;
		}
	}

	renderer->present();
	p8_memory[ADDR_DS_COLOR] = original_color;
	memory_write_short(ADDR_DS_CAMERA_X, original_camera_x);
	memory_write_short(ADDR_DS_CAMERA_Y, original_camera_y);
}

