#pragma once

#include <string>

extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
	#include <eris.h>
}

class LuaState
{
public:
	LuaState();
	~LuaState();

	bool loadProgram(std::string& program);
	unsigned int getSize();
	void serialize(unsigned char* dest);
	void deserialize(unsigned char* src);

	void run_init();
	void run_draw();
	void run_update(); // _update60()
	bool is60FPS;

private:
	lua_State* state;
};
extern LuaState* luaState;
