#include "machine_state.h"
#include "renderer.h"
#include "font.h";
#include "log.h"
#include "memory.h"

Font f;
#define DEBUGLOG MachineStateState_DEBUGLOG
Log DEBUGLOG = logger.log("MachineState");

MachineState::MachineState()
{
	this->started = getTimestamp();

	for (int p = 0; p < 8; p++) {
		for (int b = 0; b < 8; b++) {
			this->btn_countdown[p][b] = 0;
		}
	}
}

void MachineState::initialize()
{
	this->setRndSeed(rand());

	for (int i = 0; i < 8; i++) {
		p8_memory[ADDR_HW_BTN_STATES] = 0;
	}

	// Things that are not in p8's memory
	this->started = getTimestamp();
	for (int p = 0; p < 8; p++) {
		for (int b = 0; b < 8; b++) {
			this->btn_countdown[p][b] = 0;
		}
	}
}

void MachineState::processKeyEvent(KeyEvent evt)
{
	int btnBit = 1 << (int)evt.key;
	this->btn_countdown[evt.player][(int)evt.key] = 0;
	if (evt.down) {
		p8_memory[ADDR_HW_BTN_STATES + evt.player] |= btnBit;
	} else {
		p8_memory[ADDR_HW_BTN_STATES + evt.player] &= ~btnBit;
	}
}

bool MachineState::isButtonPressed(int p, P8_Key key)
{
	int btnBit = 1 << (int)key;
	return (p8_memory[ADDR_HW_BTN_STATES + p] & btnBit) > 0;
}

// for btnp - It was pressed in the last frame
bool MachineState::wasButtonPressed(int p, P8_Key btn)
{
	int cd = this->btn_countdown[p][(int)btn];
	if (cd == 0) {
		if (this->isButtonPressed(p, btn)) {
			int delay = p8_memory[ADDR_HW_BTNP_DELAY];
			if (delay == 0) {
				delay = 16;
			}
			else if (delay == 0xFF) {
				delay = -1;
			}
			this->btn_countdown[p][(int)btn] = delay;
			return true;
		}
		return false;
	}
	else if (cd == 1) {
		if (this->isButtonPressed(p, btn)) {
			int delay = p8_memory[ADDR_HW_BTNP_INTERVAL];
			if (delay == 0) {
				delay = 5;
			}
			this->btn_countdown[p][(int)btn] = 5;
			return true;
		}
		return false;
	}

	return false;
}

unsigned short MachineState::getButtonsState()
{
	return memory_read_short(ADDR_HW_BTN_STATES);
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
	auto timediff = getMillisecondsDiff(getTimestamp(), this->started);
	return (float)timediff / 1000;
}

#include <sstream>
std::string tohex(unsigned int v) {
	std::ostringstream buf;

	buf << std::hex << v;

	return buf.str();
}
unsigned int MachineState::getRnd()
{
	unsigned int low = memory_read_int(ADDR_HW_RAND_STATE);
	unsigned int high = memory_read_int(ADDR_HW_RAND_STATE+4);

	low = (low << 16) | (low >> 16);
	low += high;
	high += low;

	memory_write_int(ADDR_HW_RAND_STATE, low);
	memory_write_int(ADDR_HW_RAND_STATE + 4, high);

	return low;
}

void MachineState::setRndSeed(unsigned int n)
{
	n = n & 0x7FFFFFFF;
	if (n == 0) {
		n = 0xDEADBEEF;
	}
	unsigned int high = n;
	unsigned int low = n ^ 0xBEAD29BA;
	for (int i = 0; i < 0x20; i++) {
		low = (low << 16) | (low >> 16);
		low += high;
		high += low;
	}
	memory_write_int(ADDR_HW_RAND_STATE, low);
	memory_write_int(ADDR_HW_RAND_STATE+4, high);
}
