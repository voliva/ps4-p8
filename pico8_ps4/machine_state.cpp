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
	this->started = std::chrono::system_clock::now();

	for (int p = 0; p < 8; p++) {
		for (int b = 0; b < 8; b++) {
			this->btn_countdown[p][b] = 0;
		}
	}
}

void MachineState::initialize()
{
	// RNG Seed => https://en.wikipedia.org/wiki/Linear_congruential_generator#m_a_power_of_2,_c_=_0
	for (int i = 0; i < 8; i++) {
		p8_memory[ADDR_HW_RAND_STATE+i] = rand() % 0xFF;
	}
	// The number must be odd => The last bit must be 1
	unsigned long long seed = memory_read_long(ADDR_HW_RAND_STATE) | 0x01;
	memory_write_long(ADDR_HW_RAND_STATE, seed);

	for (int i = 0; i < 8; i++) {
		p8_memory[ADDR_HW_BTN_STATES] = 0;
	}

	// Things that are not in p8's memory
	this->started = std::chrono::system_clock::now();
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
	auto now = std::chrono::system_clock::now();
	auto timediff = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->started).count();
	return (float)timediff / 1000;
}
