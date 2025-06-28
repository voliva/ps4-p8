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

enum class Key {
	None, left, right, up, down, circle, cross, pause, quit, L2, R2
};
Key getKeyDown(SDL_Event& e);
