#include "machine_state.h"
#include "renderer.h"
#include "font.h";
#include "log.h"

Font f;
#define DEBUGLOG MachineStateState_DEBUGLOG
Log DEBUGLOG = logger.log("MachineState");

MachineState::MachineState()
{
	this->started = std::chrono::system_clock::now();
	this->color = 0;
	this->cursor.x = 0;
	this->cursor.y = 0;

	for (int p = 0; p < 8; p++) {
		this->player_btns[p] = 0;
		for (int b = 0; b < 8; b++) {
			this->btn_countdown[p][b] = 0;
		}
	}
}

void MachineState::print(std::string& text)
{
	this->print(text, this->cursor);
	// TODO scroll
}

void MachineState::print(std::string& text, SDL_Point position)
{
	int y_pos = f.print(text, position.x, position.y, renderer);
	this->cursor = position;
	this->cursor.y += y_pos;
}

int MachineState::getColor()
{
	return this->color;
}

void MachineState::setColor(int color)
{
	this->color = color;
	set_color(color);
}

void MachineState::processKeyEvent(KeyEvent evt)
{
	int btnBit = 1 << (int)evt.key;
	this->btn_countdown[evt.player][(int)evt.key] = 0;
	if (evt.down) {
		this->player_btns[evt.player] |= btnBit;
	} else {
		this->player_btns[evt.player] &= ~btnBit;
	}
}

bool MachineState::isButtonPressed(int p, P8_Key key)
{
	int btnBit = 1 << (int)key;
	return (this->player_btns[p] & btnBit) > 0;
}

// for btnp - It was pressed in the last frame
bool MachineState::wasButtonPressed(int p, P8_Key btn)
{
	int cd = this->btn_countdown[p][(int)btn];
	if (cd == 0) {
		if (this->isButtonPressed(p, btn)) {
			this->btn_countdown[p][(int)btn] = 16;
			return true;
		}
		return false;
	}
	else if (cd == 1) {
		if (this->isButtonPressed(p, btn)) {
			this->btn_countdown[p][(int)btn] = 5;
			return true;
		}
		return false;
	}

	return false;
}

int MachineState::getButtonsState()
{
	return this->player_btns[0] | (this->player_btns[1] << 8);
}

void MachineState::registerFrame()
{
	for (int p = 0; p < 8; p++) {
		for (int b = 0; b < 8; b++) {
			if (this->btn_countdown[p][b] > 0) {
				this->btn_countdown[p][b]--;
			}
		}
	}
}

float MachineState::getTime()
{
	auto now = std::chrono::system_clock::now();
	auto timediff = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->started).count();
	return (float)timediff / 1000;
}
