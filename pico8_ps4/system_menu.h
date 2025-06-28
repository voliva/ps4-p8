#pragma once

#include "events.h"
#include <vector>
#include <string>
#include <functional>

typedef struct {
	std::string label;
	std::function<void()> callback;
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
	bool pressed;
};
extern SystemMenu* activeSystemMenu;
