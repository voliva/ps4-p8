#pragma once

#include <SDL2/SDL.h>
#include <string>
#include "events.h"

class MachineState
{
public:
	SDL_Point cursor;
	void print(std::string& text);
	void print(std::string& text, SDL_Point position);
	int getColor();
	void setColor(int color);
	void processKeyEvent(KeyEvent evt);
	bool isButtonPressed(int p, P8_Key btn);
	bool wasButtonPressed(int p, P8_Key btn);
	int getButtonsState();
	void registerFrame();

private:
	int color;
	int player_btns[8]; // [player] => button mask
	int btn_countdown[8][8]; // [player] => [button] => frames for next tick. 0 = it wasn't pressed before, 1 = tick now, reset to 5.
};

extern MachineState* machineState;
