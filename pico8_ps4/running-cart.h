#pragma once
#include "cartridge.h"

enum RunningStatus {
	None,
	Loaded,
	Running,
	Restarting,
	Stopping
};

class RunningCart
{
public:
	bool load(Cartridge* cartridge);
	void run();
	void stop();
	void restart();
	void pause();
	void resume();

private:
	Cartridge* loadedCartridge;
	RunningStatus status = RunningStatus::None;
	bool paused = false;

	void runOnce();
};
extern RunningCart *runningCart;

void run_cartridge(Cartridge* r);