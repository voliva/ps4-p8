#pragma once

#include <string>

extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

class LuaState
{
public:
	LuaState();
	~LuaState();
	bool loadProgram(std::string& program);
	void run_init();
	void run_draw();
	void run_update(); // _update60()
	bool is60FPS;

private:
	lua_State* state;
};
extern LuaState* luaState;
