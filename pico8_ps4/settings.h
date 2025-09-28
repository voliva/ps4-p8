#pragma once
#include "events.h"

class Settings
{
private:
	int focusIdx;

public:
	void initialize();
	void key_down(Key k);
	void render(long long delta);
};

