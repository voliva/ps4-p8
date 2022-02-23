#include "machine_state.h"
#include "renderer.h"
#include "font.h";
#include "log.h"
#include "memory.h"
#include "lua_state.h"

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

	// Breadcrumb should not be reset when initializing
	this->breadcrumb = "";
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

	this->state = (unsigned char*)malloc(
		this->getSize() + luaState->getSize()
	);
}

unsigned int MachineState::getSize()
{
	return P8_TOTAL_MEMORY;
}

void MachineState::serialize(unsigned char* dest)
{
	memcpy(dest, p8_memory, P8_TOTAL_MEMORY);
}

void MachineState::deserialize(unsigned char* src)
{
	memcpy(p8_memory, src, P8_TOTAL_MEMORY);
}

void MachineState::saveState()
{
	memset(this->state, 0, this->getSize() + luaState->getSize());
	this->serialize(this->state);
	luaState->serialize(&this->state[this->getSize()]);
}
void MachineState::loadState()
{
	this->deserialize(this->state);
	luaState->deserialize(&this->state[this->getSize()]);
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
	if (cd == 0 || cd == 1) {
		return this->isButtonPressed(p, btn);
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
			if (this->btn_countdown[p][b] == 0 && this->isButtonPressed(p, (P8_Key)b)) {
				// Start initial countdown
				int delay = p8_memory[ADDR_HW_BTNP_DELAY];
				if (delay == 0) {
					delay = 16;
				}
				else if (delay == 0xFF) {
					delay = -1;
				}
				this->btn_countdown[p][b] = delay;
			}else if (this->btn_countdown[p][b] == 1 && this->isButtonPressed(p, (P8_Key)b)) {
				// Reset countdown to interval
				int delay = p8_memory[ADDR_HW_BTNP_INTERVAL];
				if (delay == 0) {
					delay = 5;
				}
				this->btn_countdown[p][b] = delay;
			}else if (this->btn_countdown[p][b] > 0) {
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
