#pragma once

#include <SDL2/SDL.h>
#include <string>
#include "chrono.h"
#include "events.h"

class MachineState
{
public:
	MachineState();

	void initialize();

	void processKeyEvent(KeyEvent evt);
	bool isButtonPressed(int p, P8_Key btn);
	bool wasButtonPressed(int p, P8_Key btn);
	unsigned short getButtonsState();
	void registerFrame();
	float getTime();

private:
	int btn_countdown[8][8]; // [player] => [button] => frames for next tick. 0 = it wasn't pressed before, 1 = tick now, reset to 5.
	timestamp_t started;
};

extern MachineState* machineState;
