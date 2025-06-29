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
	bool load(Cartridge* cartridge, std::string name);
	void reload(int dest, int source, int length);
	void replace(Cartridge* cartridge, std::string name);
	void run();
	void stop();
	void restart();
	void pause();
	void resume();
	void warnError();
	std::string getName();

private:
	Cartridge* loadedCartridge;
	RunningStatus status = RunningStatus::None;
	bool paused = false;
	timestamp_t lastWarning = nilTimestamp();
	std::string name;

	Cartridge* replacingCartridge;
	std::string replacingName;

	void runOnce();
	void dismissError();
};
extern RunningCart *runningCart;

void run_cartridge(Cartridge* r, std::string name);
