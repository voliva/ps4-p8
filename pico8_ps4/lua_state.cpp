#include "lua_state.h"
#include "log.h"
#include "lua_fns.h"
#include "chrono.h"
#include "running-cart.h"

#define DEBUGLOG LuaState_DEBUGLOG
Log DEBUGLOG = logger.log("LuaState");

std::string LUA_PREFIX = "__lua_";
#define PREFIXED(n) (LUA_PREFIX + n).c_str()

LuaState::LuaState()
{
	srand(std::time(NULL));

    this->state = luaL_newstate();
	this->is60FPS = false;

	luaL_requiref(this->state, LUA_GNAME, luaopen_base, 1);
	lua_pop(this->state, 1);
	luaL_requiref(this->state, PREFIXED(LUA_ERISLIBNAME), luaopen_eris, 1);
	lua_pop(this->state, 1);
	luaL_requiref(this->state, PREFIXED(LUA_TABLIBNAME), luaopen_table, 1);
	lua_pop(this->state, 1);
	luaL_requiref(this->state, PREFIXED(LUA_MATHLIBNAME), luaopen_math, 1);
	lua_pop(this->state, 1);
	luaL_requiref(this->state, PREFIXED(LUA_STRLIBNAME), luaopen_string, 1);
	lua_pop(this->state, 1);
	luaL_requiref(this->state, PREFIXED(LUA_COLIBNAME), luaopen_coroutine, 1);
	lua_pop(this->state, 1);
	luaL_requiref(this->state, PREFIXED(LUA_DBLIBNAME), luaopen_debug, 1);
	lua_pop(this->state, 1);

	load_lang_fns(this->state); // pairs, all, etc.
	load_draw_fns(this->state); // print, cls, rect, etc.
	load_sound_fns(this->state); // sfx, music, etc.
	load_machine_fns(this->state); // peek, poke, stat, extcmd, etc.
	load_math_fns(this->state); // sin, max, rnd, etc.

	lua_getglobal(this->state, "__lua_eris");
	lua_getfield(this->state, -1, "init_persist_all");
	if (lua_pcall(this->state, 0, 0, 0) != 0) {
		std::string e = lua_tostring(this->state, -1);
		DEBUGLOG << "init persist all: " << e << ENDL;
	}
	lua_pop(this->state, 1);
}

LuaState::~LuaState()
{
	lua_close(this->state);
}

unsigned int LuaState::getSize()
{
	return 1024 * 1024; // 1MB enough ?
}
void LuaState::serialize(unsigned char* dest)
{
	lua_getglobal(this->state, "__lua_eris");
	lua_getfield(this->state, -1, "persist_all");
	if (lua_pcall(this->state, 0, 1, 0) != 0) {
		std::string e = lua_tostring(this->state, -1);
		lua_pop(this->state, 1);  // pop error message from the stack *
		DEBUGLOG << "serialize: " << e << ENDL;

		return;
	}

	size_t len;
	const char* result = lua_tolstring(this->state, -1, &len);
	lua_pop(this->state, 2);

	memcpy(dest, &len, sizeof(len));
	dest += sizeof(len);
	memcpy(dest, result, len);
}
void LuaState::deserialize(unsigned char* src)
{
	size_t len;
	memcpy(&len, src, sizeof(len));
	src += sizeof(len);

	lua_getglobal(this->state, "__lua_eris");
	lua_getfield(this->state, -1, "restore_all");
	lua_pushlstring(this->state, (const char*)src, len);
	if (lua_pcall(this->state, 1, 0, 0) != 0) {
		std::string e = lua_tostring(this->state, -1);
		lua_pop(this->state, 1);  // pop error message from the stack *
		DEBUGLOG << "deserialize: " << e << ENDL;

		return;
	}
	lua_pop(this->state, 1);
}

bool LuaState::loadProgram(std::string& program)
{
	DEBUGLOG << "calling" << ENDL;
	int error = luaL_loadbuffer(this->state, program.c_str(), program.length(), "program") || lua_pcall(this->state, 0, 0, 0);

	if (error) {
		// DEBUGLOG << program << ENDL;
		std::string e = lua_tostring(this->state, -1);
		lua_pop(this->state, 1);  // pop error message from the stack *
		DEBUGLOG << e << ENDL;

		return false;
	}

	return true;
}

void LuaState::run_init()
{
	lua_getglobal(this->state, "_init");
	if (lua_isfunction(this->state, 1)) {
		if (lua_pcall(this->state, 0, 0, 0)) {
			// Error
			std::string e = lua_tostring(this->state, -1);
			lua_pop(this->state, 1);
			DEBUGLOG << e << ENDL;
			runningCart->warnError();
			return;
		}
	}
	else {
		lua_pop(this->state, 1);
	}

	// Detect whether it's running in 30fps or 60fps
	// Can't do this earlier, because some games assign _draw and/or _update in the init function
	this->is60FPS = true;
	lua_getglobal(this->state, "_update60");
	if (!lua_isfunction(this->state, 1)) {
		this->is60FPS = false;
	}
	lua_pop(this->state, 1);
}

void LuaState::run_draw()
{
	lua_getglobal(this->state, "_draw");
	if (lua_isfunction(this->state, 1)) {
		if (lua_pcall(this->state, 0, 0, 0)) {
			std::string e = lua_tostring(this->state, -1);
			lua_pop(this->state, 1);
			DEBUGLOG << e << ENDL;
			runningCart->warnError();
		}
	}
	else {
		lua_pop(this->state, 1);
	}
}

void LuaState::run_update()
{
	if (this->is60FPS) {
		lua_getglobal(this->state, "_update60");
		if (lua_pcall(this->state, 0, 0, 0)) {
			std::string e = lua_tostring(this->state, -1);
			lua_pop(this->state, 1);
			DEBUGLOG << e << ENDL;
			runningCart->warnError();
		}
	}
	else {
		lua_getglobal(this->state, "_update");
		if (lua_isfunction(this->state, 1)) {
			if (lua_pcall(this->state, 0, 0, 0)) {
				std::string e = lua_tostring(this->state, -1);
				lua_pop(this->state, 1);
				DEBUGLOG << e << ENDL;
				runningCart->warnError();
			}
		}
		else {
			lua_pop(this->state, 1);
		}
	}
}
