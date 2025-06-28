#pragma once

#include "events.h"
#include <vector>
#include <string>
#include <functional>

typedef struct {
	std::string label;
	std::function<void()> callback;
	bool confirm;
} MenuItem;

class SystemMenu
{
public:
	SystemMenu(std::vector<MenuItem> &items);
	void initialize();
	void keyDown(Key key);
	void draw();

private:
	std::vector<MenuItem> items;
	unsigned char active_index;
	bool confirming;
};
extern SystemMenu* activeSystemMenu;
