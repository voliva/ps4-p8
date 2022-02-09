#pragma once
#include "cartridge.h"
#include "chrono.h"

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
	void reload(int dest, int source, int length);
	void run();
	void stop();
	void restart();
	void pause();
	void resume();
	void warnError();

private:
	Cartridge* loadedCartridge;
	RunningStatus status = RunningStatus::None;
	bool paused = false;
	timestamp_t lastWarning = nilTimestamp();

	void runOnce();
	void dismissError();
};
extern RunningCart *runningCart;

void run_cartridge(Cartridge* r);