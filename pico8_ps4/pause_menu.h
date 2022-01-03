#pragma once

#include "events.h"

class PauseMenu
{
public:
	void initialize();
	int manageEvent(KeyEvent &e);
	void draw();

private:
	unsigned char active_index;
	bool pressed;
};
extern PauseMenu* pauseMenu;
