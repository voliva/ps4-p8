#include "events.h"
#include "log.h"

#define DEBUGLOG Events_DEBUGLOG
Log DEBUGLOG = logger.log("events");

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
	{SDL_SCANCODE_RETURN, KeyEvent{ P8_Key::pause, 0 }},
};
std::map<Uint8, P8_Key> controller_to_key = {
#ifdef __SWITCH__
	{0, P8_Key::cross},
	{3, P8_Key::cross},
	{6, P8_Key::cross},
	{1, P8_Key::circle},
	{2, P8_Key::circle},
	{7, P8_Key::circle},
	{10, P8_Key::pause},
	{11, P8_Key::pause},
	{17, P8_Key::up},
	{19, P8_Key::down},
	{16, P8_Key::left},
	{18, P8_Key::right},
	{13, P8_Key::up},
	{15, P8_Key::down},
	{12, P8_Key::left},
	{14, P8_Key::right}
#else
	{0, P8_Key::cross},
	{1, P8_Key::circle},
	{2, P8_Key::circle}, // square (So that both circle and cross can easily be pressed at the same time with the thumb)
	{3, P8_Key::cross},	 // triangle
	{4, P8_Key::circle}, // L1
	{5, P8_Key::cross},	 // R1
	{9, P8_Key::pause},
	{13, P8_Key::up},
	{14, P8_Key::down},
	{15, P8_Key::left},
	{16, P8_Key::right},
#endif
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
#ifdef __SWITCH__
	{0, Key::cross},
	{3, Key::cross},
	{1, Key::circle},
	{2, Key::circle},
	{10, Key::pause},
	{11, Key::quit},
	{17, Key::up},
	{19, Key::down},
	{16, Key::left},
	{18, Key::right},
	{13, Key::up},
	{15, Key::down},
	{12, Key::left},
	{14, Key::right},
	{8, Key::L2},
	{9, Key::R2},
#else
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
 * 11 L3
 * 12 R3
 * 17 Mid
 */
#endif
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

// SWITCH
// a b x y
// 0 1 2 3
//
// RS
// b u  d  l  r
// 5 21 23 20 22
//
// LS
// b u  d  l  r
// 4 17 19 16 18
//
// u  d  l  r
// 13 15 12 14
//
// ZL L -
// 8  6 11
//
// ZR R +
// 9  7 10
