#pragma once

#include <map>
#include <SDL2/SDL.h>

enum class P8_Key {
	left = 0, right, up, down, circle, cross, pause, reserved
};
typedef struct {
	P8_Key key;
	int player;
	bool down; // or up
} KeyEvent;

KeyEvent* mapSdlEvent(SDL_Event& e);