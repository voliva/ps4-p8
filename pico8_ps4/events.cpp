#include "events.h"

std::map<SDL_Scancode, KeyEvent> key_to_event = {
	{SDL_SCANCODE_X, KeyEvent{ P8_Key::cross, 0 }},
	{SDL_SCANCODE_V, KeyEvent{ P8_Key::cross, 0 }},
	{SDL_SCANCODE_M, KeyEvent{ P8_Key::cross, 0 }},
	{SDL_SCANCODE_Z, KeyEvent{ P8_Key::circle, 0 }},
	{SDL_SCANCODE_C, KeyEvent{ P8_Key::circle, 0 }},
	{SDL_SCANCODE_N, KeyEvent{ P8_Key::circle, 0 }},
	{SDL_SCANCODE_UP, KeyEvent{ P8_Key::up, 0 }},
	{SDL_SCANCODE_DOWN, KeyEvent{ P8_Key::down, 0 }},
	{SDL_SCANCODE_LEFT, KeyEvent{ P8_Key::left, 0 }},
	{SDL_SCANCODE_RIGHT, KeyEvent{ P8_Key::right, 0 }},
};
std::map<Uint8, P8_Key> controller_to_key = {
	{0, P8_Key::cross},
	{1, P8_Key::circle},
	{13, P8_Key::up},
	{14, P8_Key::down},
	{15, P8_Key::left},
	{16, P8_Key::right},
};

KeyEvent* mapSdlEvent(SDL_Event& e)
{
	switch (e.type) {
	case SDL_KEYDOWN:
		if (e.key.repeat) {
			break;
		}
		else if (key_to_event.count(e.key.keysym.scancode)) {
			KeyEvent base = key_to_event[e.key.keysym.scancode];
			return new KeyEvent{
				base.key,
				base.player,
				true
			};
		}
		break;
	case SDL_KEYUP:
		if (key_to_event.count(e.key.keysym.scancode)) {
			KeyEvent base = key_to_event[e.key.keysym.scancode];
			return new KeyEvent{
				base.key,
				base.player,
				false
			};
		}
		break;
	case SDL_JOYBUTTONDOWN:
		if (controller_to_key.count(e.jbutton.button)) {
			P8_Key key = controller_to_key[e.jbutton.button];
			return new KeyEvent{
				key,
				0, // TODO player e.jbutton.which, <= This is not index, it's a device id
				true
			};
		}
		break;
	case SDL_JOYBUTTONUP:
		if (controller_to_key.count(e.jbutton.button)) {
			P8_Key key = controller_to_key[e.jbutton.button];
			return new KeyEvent{
				key,
				0, // TODO player e.jbutton.which,
				false
			};
		}
		break;
	}

	return NULL;
}

std::map<SDL_Scancode, Key> sdl_key_to_sys_key = {
	{SDL_SCANCODE_X, Key::cross},
	{SDL_SCANCODE_V, Key::cross},
	{SDL_SCANCODE_M, Key::cross},
	{SDL_SCANCODE_Z, Key::circle},
	{SDL_SCANCODE_C, Key::circle},
	{SDL_SCANCODE_N, Key::circle},
	{SDL_SCANCODE_UP, Key::up},
	{SDL_SCANCODE_DOWN, Key::down},
	{SDL_SCANCODE_LEFT, Key::left},
	{SDL_SCANCODE_RIGHT, Key::right},
	{SDL_SCANCODE_RETURN, Key::pause},
	{SDL_SCANCODE_KP_4, Key::L2},
	{SDL_SCANCODE_KP_6, Key::R2},
};
std::map<Uint8, Key> controller_to_sys_key = {
	{0, Key::cross},
	{1, Key::circle},
	{9, Key::pause},
	{13, Key::up},
	{14, Key::down},
	{15, Key::left},
	{16, Key::right},
	{18, Key::L2},
	{19, Key::R2},
	/*
	* 2 square
	* 3 triangle
	* 4 L1
	* 5 R1
	* 11 L3
	* 12 R3
	* 17 Mid
	*/
};

Key getKeyDown(SDL_Event& e) {
	switch (e.type) {
	case SDL_KEYDOWN:
		if (e.key.repeat) {
			break;
		}
		if (sdl_key_to_sys_key.count(e.key.keysym.scancode)) {
			return sdl_key_to_sys_key[e.key.keysym.scancode];
		}
		break;
	case SDL_JOYBUTTONDOWN:
		if (controller_to_sys_key.count(e.jbutton.button)) {
			return controller_to_sys_key[e.jbutton.button];
		}
		break;
	}

	return Key::None;
}
