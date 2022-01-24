#include "lua_fns.h"
#include "audio.h"

int sfx(lua_State* L) {
	int n = luaL_checkinteger(L, 1);
	int channel = luaL_optinteger(L, 2, -1);
	int offset = luaL_optinteger(L, 3, 0);
	int length = luaL_optinteger(L, 4, 31);

	if (n == -1 && channel >= 0) {
		audioManager->stopChannel(channel);
		return 0;
	}
	else if (n < 0) {
		return 0;
	}

	if (channel == -2) {
		audioManager->stopSfx(n);
		return 0;
	}

	audioManager->playSfx(n, channel, offset, length);

	return 0;
}

int music(lua_State* L) {
	int n = luaL_optinteger(L, 1, 0);
	int fade = luaL_optinteger(L, 2, 0);
	int channelmask = luaL_optinteger(L, 3, 0);

	if (n == -1) {
		audioManager->stopMusic();
	}
	else {
		audioManager->playMusic(n, channelmask);
	}

	return 0;
}

void load_sound_fns(lua_State* L)
{
	register_fn(L, "sfx", sfx);
	register_fn(L, "music", music);
}
