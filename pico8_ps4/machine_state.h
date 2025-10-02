#pragma once

#include <SDL2/SDL.h>
#include <string>
#include "chrono.h"
#include "events.h"

#define MAX_RND ((unsigned int)0xffffffff)
class MachineState
{
public:
	MachineState();

	void initialize();
	unsigned int getSize();
	void serialize(unsigned char* dest);
	void deserialize(unsigned char* src);

	bool controlInverted;
	void processKeyEvent(KeyEvent evt);
	bool isButtonPressed(int p, P8_Key btn);
	bool wasButtonPressed(int p, P8_Key btn);
	unsigned short getButtonsState();
	void registerFrame();
	float getTime();

	unsigned int getRnd();
	void setRndSeed(unsigned int n);

	std::string breadcrumb;

private:
	int btn_countdown[8][8]; // [player] => [button] => frames for next tick. 0 = it wasn't pressed before, 1 = tick now, reset to 5.
	int frames;
};

extern MachineState* machineState;
