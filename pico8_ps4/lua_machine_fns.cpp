#include "lua_fns.h"
#include "memory.h"
#include "machine_state.h"
#include "audio.h"
#include "running-cart.h"
#include "saves.h"

int peek(lua_State* L)
{
	unsigned short addr = luaL_checkinteger(L, 1);
	int n = luaL_optinteger(L, 2, 1);

	for (int i = 0; i < n; i++) {
		lua_pushinteger(L, p8_memory[addr + i]);
	}

	return n;
}

int peek2(lua_State* L)
{
	unsigned short addr = luaL_checkinteger(L, 1);
	int n = luaL_optinteger(L, 2, 1);

	for (int i = 0; i < n; i++) {
		short value = memory_read_short(addr + i * 2);
		lua_pushinteger(L, value);
	}

	return n;
}

int peek4(lua_State* L)
{
	unsigned short addr = luaL_checkinteger(L, 1);
	int n = luaL_optinteger(L, 2, 1);

	for (int i = 0; i < n; i++) {
		int raw = memory_read_int(addr + i * 4);
		lua_pushnumber(L, raw);
	}

	return n;
}

int time(lua_State* L) {
	lua_pushnumber(L, fix16_from_float(machineState->getTime()));
	return 1;
}

void poke_memory(unsigned short addr, unsigned char value) {
	p8_memory[addr] = value;
}
int poke(lua_State* L) {
	unsigned short addr = luaL_checkinteger(L, 1) & 0x0FFFF;
	unsigned char value = luaL_optinteger(L, 2, 0) & 0x0FF;

	poke_memory(addr, value);

	for (int i = 3; lua_isinteger(L, i); i++) {
		value = lua_tointeger(L, i) & 0x0FF;
		addr++;
		poke_memory(addr, value);
	}

	return 0;
}
int poke2(lua_State* L) {
	unsigned short addr = luaL_checkinteger(L, 1) & 0x0FFFF;
	unsigned short value = luaL_optinteger(L, 2, 0) & 0x0FFFF;

	memory_write_short(addr, value);

	for (int i = 3; lua_isinteger(L, i); i++) {
		value = lua_tointeger(L, i) & 0x0FFFF;
		addr+=2;
		memory_write_short(addr, value);
	}

	return 0;
}
int poke4(lua_State* L) {
	unsigned short addr = luaL_checkinteger(L, 1) & 0x0FFFF;
	unsigned int value = luaL_optnumber(L, 2, 0);

	memory_write_int(addr, value);

	for (int i = 3; lua_isnumber(L, i); i++) {
		value = lua_tonumber(L, i);
		addr+=4;
		memory_write_int(addr, value);
	}

	return 0;
}

int btnp(lua_State* L) {
	if (lua_gettop(L) >= 1) {
		int i = lua_tointeger(L, 1);
		int p = luaL_optinteger(L, 2, 0);

		lua_pushboolean(L, machineState->wasButtonPressed(p, (P8_Key)i));
	}
	else {
		lua_pushinteger(L, machineState->getButtonsState());
	}
	return 1;
}

int btn(lua_State* L) {
	if (lua_gettop(L) >= 1) {
		int i = lua_tointeger(L, 1);
		int p = luaL_optinteger(L, 2, 0);

		lua_pushboolean(L, machineState->isButtonPressed(p, (P8_Key)i));
	}
	else {
		lua_pushinteger(L, machineState->getButtonsState());
	}
	return 1;
}

int extcmd(lua_State* L) {
	std::string cmd = luaL_checkstring(L, 1);

	if (cmd == "reset") {
		runningCart->restart();
	}
	else if (cmd == "pause") {
		runningCart->pause();
	}
	else if (cmd == "shutdown") {
		runningCart->stop();
	}
	else {
		alert_todo("extcmd(" + cmd + ")");
	}

	return 0;
}

int dset(lua_State* L) {
	int index = lua_tointeger(L, 1);
	lua_Number value = lua_tonumber(L, 2);

	saveManager->write(index, value);

	return 0;
}

int dget(lua_State* L) {
	int index = luaL_checkinteger(L, 1);

	lua_pushnumber(L, saveManager->read(index));

	return 1;
}

int cartdata(lua_State* L) {
	std::string name = luaL_checkstring(L, 1);

	saveManager->open(name);

	return 0;
}

int memset(lua_State* L) {
	unsigned short destaddr = lua_tointeger(L, 1);
	unsigned char val = lua_tointeger(L, 2);
	unsigned short len = lua_tointeger(L, 3);

	for (int i = 0; i < len; i++) {
		poke_memory(destaddr + i, val);
	}

	return 0;
}

int stat(lua_State* L) {
	int n = luaL_checkinteger(L, 1);

	if (n == 6) {
		lua_pushstring(L, machineState->breadcrumb.c_str());
		return 1;
	}

	if (16 <= n && n <= 26) {
		// Deprecated, bump to 46..56
		n += 30;
	}

	if (n == 30) {
		// boolean whether key has been pressed -> Return false
		lua_pushboolean(L, false);
		return 1;
	}
	if (31 <= n && n <= 39) {
		// Mouse + keyboard support not enabled on PS4
		alert_todo("stat(mouse + keyboard)");
		lua_pushinteger(L, 0);
		return 1;
	}
	if (46 <= n && n <= 49) {
		unsigned char c = n - 46;
		// Get SFX playing on channel c
		lua_pushinteger(L, audioManager->channels[c].sfx);
		return 1;
	}
	if (50 <= n && n <= 53) {
		unsigned char c = n - 50;
		// Get Note number playing on channel c
		lua_pushinteger(L, audioManager->getCurrentIndex(c));
		return 1;
	}
	if (n == 54) {
		lua_pushinteger(L, audioManager->getActivePattern());
		return 1;
	}

	alert_todo("stat(" + std::to_string(n) + ")");

	lua_pushinteger(L, 0);
	return 1;
}

int reload(lua_State* L) {
	if (lua_gettop(L) == 0) {
		runningCart->reload(0, 0, 0x4300);
		return 0;
	}
	if (lua_gettop(L) == 4) {
		alert_todo("reload from external cart");
		return 0;
	}

	unsigned short destaddr = luaL_checkinteger(L, 1);
	unsigned short sourceaddr = luaL_checkinteger(L, 2);
	unsigned short len = luaL_checkinteger(L, 3);
	runningCart->reload(destaddr, sourceaddr, len);
	return 0;
}

int reset(lua_State* L) {
	alert_todo("reset (undocumented)");
	runningCart->reload(0, 0, 0x4300);

	return 0;
}

int trace(lua_State* L) {
	alert_todo("trace");

	std::string str = luaL_optstring(L, 2, "TODO");

	lua_pushstring(L, str.c_str());

	return 1;
}

int run(lua_State* L) {
	std::string val = luaL_optstring(L, 1, "");

	machineState->breadcrumb = val;
	runningCart->restart();

	return 0;
}

int noop(lua_State* L) {
	alert_todo("noop");
	return 0;
}

int _memcpy(lua_State* L) {
	unsigned short destaddr = luaL_checkinteger(L, 1);
	unsigned short sourceaddr = luaL_checkinteger(L, 2);
	unsigned short len = luaL_checkinteger(L, 3);

	memcpy(&p8_memory[destaddr], &p8_memory[sourceaddr], len);
	return 0;
}

void load_machine_fns(lua_State* L)
{
	register_fn(L, "btnp", btnp);
	register_fn(L, "btn", btn);
	register_fn(L, "noop", noop);
	register_fn(L, "time", time);
	register_fn(L, "t", time);
	register_fn(L, "peek", peek);
	register_fn(L, "peek2", peek2);
	register_fn(L, "peek4", peek4);
	register_fn(L, "poke", poke);
	register_fn(L, "poke2", poke2);
	register_fn(L, "poke4", poke4);
	register_fn(L, "extcmd", extcmd);
	register_fn(L, "dget", dget);
	register_fn(L, "dset", dset);
	register_fn(L, "cartdata", cartdata);
	register_fn(L, "memset", memset);
	register_fn(L, "stat", stat);
	register_fn(L, "trace", trace);
	register_fn(L, "reload", reload);
	register_fn(L, "reset", reset);
	register_fn(L, "run", run);
	register_fn(L, "menuitem", noop);
	register_fn(L, "memcpy", _memcpy);
}