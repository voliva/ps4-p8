#pragma once

#include <string>
#include "log.h"

extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#ifndef DEBUGLOG
#define DEBUGLOG LuaFns_DEBUGLOG
#endif
extern Log DEBUGLOG;

// Globals
void alert_todo(std::string key);
void register_fn(lua_State* L, std::string name, lua_CFunction fn);
void register_lua_fn(lua_State* L, std::string name, std::string code);

void load_lang_fns(lua_State* L); // pairs, all, etc.
void load_draw_fns(lua_State* L); // print, cls, rect, etc.
void load_sound_fns(lua_State* L); // sfx, music, etc.
void load_machine_fns(lua_State* L); // peek, poke, stat, extcmd, etc.
void load_math_fns(lua_State* L); // sin, max, rnd, etc.
