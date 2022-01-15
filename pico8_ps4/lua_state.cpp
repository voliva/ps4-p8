#include "lua_state.h"
#include "log.h"
#include <math.h>
#include <stdlib.h>
#include <sstream>
#include "renderer.h"
#include "machine_state.h"
#include "audio.h"
#include "memory.h"
#include "font.h"
#include "running-cart.h"

#define DEBUGLOG LuaState_DEBUGLOG
Log DEBUGLOG = logger.log("LuaState");

int rnd(lua_State* L);
int flr(lua_State* L);
int ceil(lua_State* L);
int srand(lua_State* L);
int tostr(lua_State* L);
int sub(lua_State* L);
int btn(lua_State* L);
int btnp(lua_State* L);
int cls(lua_State* L);
int spr(lua_State* L);
int sspr(lua_State* L);
int map(lua_State* L);
int mget(lua_State* L);
int mset(lua_State* L);
int cursor(lua_State* L);
int print(lua_State* L);
int color(lua_State* L);
int tonum(lua_State* L);
int add(lua_State* L);
int deli(lua_State* L);
int noop(lua_State* L);
int time(lua_State* L);
int type(lua_State* L);
int pset(lua_State* L);
int line(lua_State* L);
int rect(lua_State* L);
int rectfill(lua_State* L);
int circ(lua_State* L);
int circfill(lua_State* L);
int oval(lua_State* L);
int ovalfill(lua_State* L);
int sfx(lua_State* L);
int music(lua_State* L);
int poke(lua_State* L);
int pal(lua_State* L);
int palt(lua_State* L);
int camera(lua_State* L);
int sqrt(lua_State* L);
int cos(lua_State* L);
int sin(lua_State* L);
int atan2(lua_State* L);
int extcmd(lua_State* L);
int fget(lua_State* L);
int fset(lua_State* L);
int dget(lua_State* L);
int shr(lua_State* L);
int shl(lua_State* L);
int printh(lua_State* L);
int sget(lua_State* L);
int sset(lua_State* L);
int clip(lua_State* L);

LuaState::LuaState()
{
	srand(std::time(NULL));

    this->state = luaL_newstate();
	this->is60FPS = false;

	lua_pushcfunction(this->state, rnd);
	lua_setglobal(this->state, "rnd");
	lua_pushcfunction(this->state, flr);
	lua_setglobal(this->state, "flr");
	lua_pushcfunction(this->state, ceil);
	lua_setglobal(this->state, "ceil");
	lua_pushcfunction(this->state, srand);
	lua_setglobal(this->state, "srand");
	lua_pushcfunction(this->state, tostr);
	lua_setglobal(this->state, "tostr");
	lua_pushcfunction(this->state, sub);
	lua_setglobal(this->state, "sub");
	lua_pushcfunction(this->state, btnp);
	lua_setglobal(this->state, "btnp");
	lua_pushcfunction(this->state, btn);
	lua_setglobal(this->state, "btn");
	lua_pushcfunction(this->state, cls);
	lua_setglobal(this->state, "cls");
	lua_pushcfunction(this->state, spr);
	lua_setglobal(this->state, "spr");
	lua_pushcfunction(this->state, sspr);
	lua_setglobal(this->state, "sspr");
	lua_pushcfunction(this->state, map);
	lua_setglobal(this->state, "map");
	lua_pushcfunction(this->state, mget);
	lua_setglobal(this->state, "mget");
	lua_pushcfunction(this->state, mset);
	lua_setglobal(this->state, "mset");
	lua_pushcfunction(this->state, cursor);
	lua_setglobal(this->state, "cursor");
	lua_pushcfunction(this->state, print);
	lua_setglobal(this->state, "print");
	lua_pushcfunction(this->state, color);
	lua_setglobal(this->state, "color");
	lua_pushcfunction(this->state, tonum);
	lua_setglobal(this->state, "tonum");
	lua_pushcfunction(this->state, add);
	lua_setglobal(this->state, "add");
	lua_pushcfunction(this->state, deli);
	lua_setglobal(this->state, "deli");
	lua_pushcfunction(this->state, noop);
	lua_setglobal(this->state, "menuitem");
	lua_pushcfunction(this->state, sfx);
	lua_setglobal(this->state, "sfx");
	lua_pushcfunction(this->state, music);
	lua_setglobal(this->state, "music");
	lua_pushcfunction(this->state, time);
	lua_setglobal(this->state, "time");
	lua_pushcfunction(this->state, type);
	lua_setglobal(this->state, "type");
	lua_pushcfunction(this->state, pset);
	lua_setglobal(this->state, "pset");
	lua_pushcfunction(this->state, line);
	lua_setglobal(this->state, "line");
	lua_pushcfunction(this->state, rect);
	lua_setglobal(this->state, "rect");
	lua_pushcfunction(this->state, rectfill);
	lua_setglobal(this->state, "rectfill");
	lua_pushcfunction(this->state, circ);
	lua_setglobal(this->state, "circ");
	lua_pushcfunction(this->state, circfill);
	lua_setglobal(this->state, "circfill");
	lua_pushcfunction(this->state, oval);
	lua_setglobal(this->state, "oval");
	lua_pushcfunction(this->state, ovalfill);
	lua_setglobal(this->state, "ovalfill");
	lua_pushcfunction(this->state, poke);
	lua_setglobal(this->state, "poke");
	lua_pushcfunction(this->state, pal);
	lua_setglobal(this->state, "pal");
	lua_pushcfunction(this->state, palt);
	lua_setglobal(this->state, "palt");
	lua_pushcfunction(this->state, camera);
	lua_setglobal(this->state, "camera");
	lua_pushcfunction(this->state, sqrt);
	lua_setglobal(this->state, "sqrt");
	lua_pushcfunction(this->state, cos);
	lua_setglobal(this->state, "cos");
	lua_pushcfunction(this->state, sin);
	lua_setglobal(this->state, "sin");
	lua_pushcfunction(this->state, atan2);
	lua_setglobal(this->state, "atan2");
	lua_pushcfunction(this->state, extcmd);
	lua_setglobal(this->state, "extcmd");
	lua_pushcfunction(this->state, fget);
	lua_setglobal(this->state, "fget");
	lua_pushcfunction(this->state, fset);
	lua_setglobal(this->state, "fset");
	lua_pushcfunction(this->state, dget);
	lua_setglobal(this->state, "dget");
	lua_pushcfunction(this->state, noop);
	lua_setglobal(this->state, "dset");
	lua_pushcfunction(this->state, noop);
	lua_setglobal(this->state, "cartdata");
	lua_pushcfunction(this->state, shr);
	lua_setglobal(this->state, "shr");
	lua_pushcfunction(this->state, shl);
	lua_setglobal(this->state, "shl");
	lua_pushcfunction(this->state, printh);
	lua_setglobal(this->state, "printh");
	lua_pushcfunction(this->state, sget);
	lua_setglobal(this->state, "sget");
	lua_pushcfunction(this->state, sset);
	lua_setglobal(this->state, "sset");
	lua_pushcfunction(this->state, clip);
	lua_setglobal(this->state, "clip");

	// It needs to count from the end of the table in case the elements get removed in-between
	std::string all =
		"function all(t) \
			if t == nil then return function() end end \
			local n = #t \
			return function() \
				local v = nil \
				while n >= 0 and v == nil do \
					v = t[#t-n] \
					n = n - 1 \
				end \
				return v \
			end \
		end";
	luaL_loadbuffer(this->state, all.c_str(), all.length(), "all");
	lua_pcall(this->state, 0, 0, 0);

	std::string min =
		"function min(first, second) \
			second = second or 0 \
			if first < second then return first else return second end \
		end";
	luaL_loadbuffer(this->state, min.c_str(), min.length(), "min");
	lua_pcall(this->state, 0, 0, 0);

	std::string max =
		"function max(first, second) \
			second = second or 0 \
			if first > second then return first else return second end \
		end";
	luaL_loadbuffer(this->state, max.c_str(), max.length(), "max");
	lua_pcall(this->state, 0, 0, 0);

	std::string foreach =
		"function foreach(tbl, fn) \
			for v in all(tbl) do \
				fn(v) \
			end \
		end";
	luaL_loadbuffer(this->state, foreach.c_str(), foreach.length(), "foreach");
	lua_pcall(this->state, 0, 0, 0);

	std::string mid =
		"function mid(a, b, c) \
			if b <= a and a <= c then return a end \
			if c <= a and a <= b then return a end \
			if a <= b and b <= c then return b end \
			if c <= b and b <= a then return b end \
			return c \
		end";
	luaL_loadbuffer(this->state, mid.c_str(), mid.length(), "mid");
	lua_pcall(this->state, 0, 0, 0);

	std::string abs =
		"function abs(a) \
			if a < 0 then return -a end \
			return a \
		end";
	luaL_loadbuffer(this->state, abs.c_str(), abs.length(), "abs");
	lua_pcall(this->state, 0, 0, 0);

	std::string split =
		"function split(str, separator, convert_numbers) \
			separator = separator or \",\" \n \
			convert_numbers = convert_numbers ~= false \n \
			local result = {} \n \
			if separator == \"\" then \
				for i=1,#str do \
					add(result, sub(str, i, i)) \
				end \
			else \n \
				local current = \"\" \n \
				for i=1,#str do \n \
					local c = sub(str, i, i) \n \
					if c == separator then \n \
						add(result, current) \n \
						current = \"\" \n \
					else \n \
						current = current..c \n \
					end \n \
				end \n \
				add(result, current) \
			end \n \
			for i=1,#result do \n \
				local value = tonum(result[i]) \n \
				if value ~= nil then \
					result[i] = value \
				end \
			end \
			return result \
		end";
	luaL_loadbuffer(this->state, split.c_str(), split.length(), "split");
	lua_pcall(this->state, 0, 0, 0);

	std::string del =
		"function del(table, value) \
			for i=1,#table do \n \
				if table[i] == value then \n \
					return deli(table, i) \n \
				end \
			end \
		end";
	luaL_loadbuffer(this->state, del.c_str(), del.length(), "del");
	lua_pcall(this->state, 0, 0, 0);

	std::string sgn =
		"function sgn(value) \
			if value >= 0 then return 1 else return -1 end \
		end";
	luaL_loadbuffer(this->state, sgn.c_str(), sgn.length(), "sgn");
	lua_pcall(this->state, 0, 0, 0);

	std::string count =
		"function count(table, value) \
			local t=0 \
			for i=1,#table do \n \
				if value == nil and table[i] ~= nil then \
					t = t + 1\
				elseif table[i] == value then \n \
					t = t + 1 \n \
				end \
			end \
			return t \
		end";
	luaL_loadbuffer(this->state, count.c_str(), count.length(), "count");
	lua_pcall(this->state, 0, 0, 0);

	// DEBUGLOG << program << ENDL;
	/*std::string e = lua_tostring(this->state, -1);
	DEBUGLOG << e << ENDL;*/
}

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
	int n = luaL_checkinteger(L, 1);
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

int add(lua_State* L) {
	int length = luaL_len(L, 1);
	int pos = luaL_optinteger(L, 3, length + 1);

	// Shift all values
	for (int i = length + 1; i > pos && i > 0; i--) {
		lua_geti(L, 1, i - 1);
		lua_seti(L, 1, i);
	}
	lua_copy(L, 2, -1);
	lua_seti(L, 1, pos);
	lua_copy(L, 2, -1);

	return 1;
}

int deli(lua_State* L) {
	int length = luaL_len(L, 1);
	int index = luaL_optinteger(L, 2, length);

	lua_geti(L, 1, index); // Push value to the stack

	// Shift all values
	for (int i = index; i < length; i++) {
		lua_geti(L, 1, i + 1);
		lua_seti(L, 1, i);
	}

	// Remove last element
	lua_pushnil(L);
	lua_seti(L, 1, length);

	return 1;
}

int pset(lua_State* L) {
	int x = luaL_checknumber(L, 1);
	int y = luaL_checknumber(L, 2);
	int col = luaL_optnumber(L, 3, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_point(x, y);

	return 0;
}

int line(lua_State* L) {
	int x0 = luaL_checknumber(L, 1);
	int y0 = luaL_checknumber(L, 2);
	int x1 = luaL_checknumber(L, 3);
	int y1 = luaL_checknumber(L, 4);
	int col = luaL_optnumber(L, 5, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_line(x0, y0, x1, y1);

	return 0;
}

int rectfill(lua_State* L) {
	int x0 = luaL_checknumber(L, 1);
	int y0 = luaL_checknumber(L, 2);
	int x1 = luaL_checknumber(L, 3);
	int y1 = luaL_checknumber(L, 4);
	int col = luaL_optnumber(L, 5, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_rectangle(x0, y0, x1, y1, true);

	return 0;
}

int rect(lua_State* L) {
	int x0 = luaL_checknumber(L, 1);
	int y0 = luaL_checknumber(L, 2);
	int x1 = luaL_checknumber(L, 3);
	int y1 = luaL_checknumber(L, 4);
	int col = luaL_optnumber(L, 5, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_rectangle(x0, y0, x1, y1, false);

	return 0;
}

int ovalfill(lua_State* L) {
	int x0 = luaL_checknumber(L, 1);
	int y0 = luaL_checknumber(L, 2);
	int x1 = luaL_checknumber(L, 3);
	int y1 = luaL_checknumber(L, 4);
	int col = luaL_optnumber(L, 5, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_oval(x0, y0, x1, y1, true);

	return 0;
}

int oval(lua_State* L) {
	int x0 = luaL_checknumber(L, 1);
	int y0 = luaL_checknumber(L, 2);
	int x1 = luaL_checknumber(L, 3);
	int y1 = luaL_checknumber(L, 4);
	int col = luaL_optnumber(L, 5, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_oval(x0, y0, x1, y1, false);

	return 0;
}

int circfill(lua_State* L) {
	int x = luaL_checknumber(L, 1);
	int y = luaL_checknumber(L, 2);
	int r = luaL_optnumber(L, 3, 4);
	int col = luaL_optnumber(L, 4, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_oval(x-r, y-r, x+r, y+r, true);

	return 0;
}

int circ(lua_State* L) {
	int x = luaL_checknumber(L, 1);
	int y = luaL_checknumber(L, 2);
	int r = luaL_optnumber(L, 3, 4);
	int col = luaL_optnumber(L, 4, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_oval(x - r, y - r, x + r, y + r, false);

	return 0;
}

int type(lua_State* L) {
	if (lua_isnumber(L, 1)) {
		lua_pushstring(L, "number");
	} else if (lua_isstring(L, 1)) {
		lua_pushstring(L, "string");
	}
	else if (lua_isboolean(L, 1)) {
		lua_pushstring(L, "boolean");
	}
	else if (lua_istable(L, 1)) {
		lua_pushstring(L, "table");
	}
	else if (lua_isnil(L, 1)) {
		lua_pushstring(L, "nil");
	}
	else {
		lua_pushstring(L, "unknown");
	}
	return 1;
}

int time(lua_State* L) {
	lua_pushnumber(L, machineState->getTime());
	return 1;
}

LuaState::~LuaState()
{
	lua_close(this->state);
}

bool LuaState::loadProgram(std::string& program)
{
    int error = luaL_loadbuffer(this->state, program.c_str(), program.length(), "program") || lua_pcall(this->state, 0, 0, 0);

	if (error) {
		// DEBUGLOG << program << ENDL;
		std::string e = lua_tostring(this->state, -1);
		lua_pop(this->state, 1);  // pop error message from the stack *
		DEBUGLOG << e << ENDL;

		return false;
	}

	// _draw or _update must exist
	bool drawExists = true;
	lua_getglobal(this->state, "_draw");
	if (!lua_isfunction(this->state, 1)) {
		drawExists = false;
	}
	lua_pop(this->state, 1);

	bool updateExists = true;
	lua_getglobal(this->state, "_update");
	if (!lua_isfunction(this->state, 1)) {
		updateExists = false;
	}
	lua_pop(this->state, 1);

	bool update60Exists = true;
	this->is60FPS = true;
	lua_getglobal(this->state, "_update60");
	if (!lua_isfunction(this->state, 1)) {
		update60Exists = false;
		this->is60FPS = false;
	}
	lua_pop(this->state, 1);

	if (!drawExists && !updateExists && !update60Exists) {
		DEBUGLOG << "_draw, _update nor _update60 don't exist" << ENDL;
		return false;
	}

	return true;
}

void LuaState::run_init()
{
	lua_getglobal(this->state, "_init");
	if (lua_isfunction(this->state, 1)) {
		if (lua_pcall(this->state, 0, 0, 0)) {
			std::string e = lua_tostring(this->state, -1);
			lua_pop(this->state, 1);
			DEBUGLOG << e << ENDL;
		}
	}
	else {
		lua_pop(this->state, 1);
	}
}

void LuaState::run_draw()
{
	lua_getglobal(this->state, "_draw");
	if (lua_isfunction(this->state, 1)) {
		if (lua_pcall(this->state, 0, 0, 0)) {
			std::string e = lua_tostring(this->state, -1);
			lua_pop(this->state, 1);
			DEBUGLOG << e << ENDL;
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
		}
	}
	else {
		lua_getglobal(this->state, "_update");
		if (lua_isfunction(this->state, 1)) {
			if (lua_pcall(this->state, 0, 0, 0)) {
				std::string e = lua_tostring(this->state, -1);
				lua_pop(this->state, 1);
				DEBUGLOG << e << ENDL;
			}
		}
		else {
			lua_pop(this->state, 1);
		}
	}
}

int poke(lua_State* L) {
	unsigned short addr = luaL_checkinteger(L, 1) & 0x0FFFF;
	unsigned char value = luaL_optinteger(L, 2, 0) & 0x0FF;

	audioManager->poke(addr, value);
	p8_memory[addr] = value;

	for (int i = 3; lua_isinteger(L, i); i++) {
		value = lua_tointeger(L, 2) & 0x0FF;
		addr++;
		audioManager->poke(addr, value);
		p8_memory[addr] = value;
	}

	return 0;
}

int string_to_num(std::string& str, short* shortval, double* doubleval);
int rnd(lua_State* L) {
	double max = 0;
	if (lua_isstring(L, 1)) {
		std::string str = luaL_checkstring(L, 1);
		short intval = 0;
		int r = string_to_num(str, &intval, &max);
		if (r == 0) {
			max = 0; // Ok? - Undocumented, taken experimentally from pico-8
		}
		else if (r == 1) {
			max = intval;
		}
	}
	else {
		max = luaL_optnumber(L, 1, 1);
	}
	double result = std::rand() * max / RAND_MAX;
	lua_pushnumber(L, result);
	return 1;
}

int flr(lua_State* L) {
	double num = luaL_checknumber(L, 1);
	lua_pushinteger(L, floor(num));
	return 1;
}

int ceil(lua_State* L) {
	double num = luaL_checknumber(L, 1);
	lua_pushinteger(L, ceil(num));
	return 1;
}

int srand(lua_State* L) {
	double num = luaL_checknumber(L, 1);
	std::srand(num);
	return 0;
}

// TODO https://pico-8.fandom.com/wiki/Tostr decimal hex?
int tostr(lua_State* L) {
	double num = lua_tonumber(L, 1);
	bool useHex = false;
	if (lua_isboolean(L, 2)) {
		useHex = lua_toboolean(L, 2);
	}

	std::ostringstream buf;
	if (useHex) {
		buf << "0x";
		if (num < 0x1000) {
			buf << "0";
		}
		if (num < 0x100) {
			buf << "0";
		}
		if (num < 0x10) {
			buf << "0";
		}
		buf << std::hex;
		buf << (int)num;
	}
	else {
		buf << num;
	}

	lua_pushstring(L, buf.str().c_str());
	return 1;
}

int sub(lua_State* L) {
	std::string s = luaL_checkstring(L, 1);
	int start = luaL_checkinteger(L, 2);
	int end = luaL_optinteger(L, 3, -1);

	int count = 0;
	if (end >= 0) {
		count = end - start + 1;
	}
	else {
		count = s.length() + end + 1 - start + 1;
	}

	std::string result = s.substr(start - 1, count);

	lua_pushstring(L, result.c_str());
	return 1;
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

int cls(lua_State* L) {
	int col = luaL_optinteger(L, 1, 0);

	renderer->clear_screen(col);
	p8_memory[ADDR_DS_CURSOR_X] = 0;
	p8_memory[ADDR_DS_CURSOR_Y] = 0;

	return 0;
}

int spr(lua_State* L) {
	int n = luaL_checknumber(L, 1);
	int x = luaL_checknumber(L, 2);
	int y = luaL_checknumber(L, 3);
	int w = luaL_optnumber(L, 4, 1.0) * 8;
	int h = luaL_optnumber(L, 5, 1.0) * 8;
	int flip_x = lua_toboolean(L, 6);
	int flip_y = lua_toboolean(L, 7);

	renderer->draw_sprite(n, x, y, w, h, flip_x, flip_y);

	return 0;
}

// TODO stretch+flip
int sspr(lua_State* L) {
	int sx = luaL_checknumber(L, 1);
	int sy = luaL_checknumber(L, 2);
	int sw = luaL_checknumber(L, 3);
	int sh = luaL_checknumber(L, 4);
	int dx = luaL_checknumber(L, 5);
	int dy = luaL_checknumber(L, 6);
	int dw = luaL_optnumber(L, 7, sw);
	int dh = luaL_optnumber(L, 8, sh);
	int flip_x = lua_toboolean(L, 9);
	int flip_y = lua_toboolean(L, 10);

	renderer->draw_from_spritesheet(sx, sy, sw, sh, dx, dy, dw, dh, flip_x, flip_y);

	return 0;
}

int map(lua_State* L) {
	int cellx = luaL_checknumber(L, 1);
	int celly = luaL_checknumber(L, 2);
	int sx = luaL_checknumber(L, 3);
	int sy = luaL_checknumber(L, 4);
	int cellw = luaL_checknumber(L, 5);
	int cellh = luaL_checknumber(L, 6);
	int layer = luaL_optinteger(L, 7, 0);

	renderer->draw_map(cellx, celly, sx, sy, cellw, cellh, layer);

	return 0;
}

int mget(lua_State* L) {
	int cellx = luaL_optnumber(L, 1, 0);
	int celly = luaL_optnumber(L, 2, 0);

	int row_offset = ADDR_MAP + celly * 128;
	if (celly >= 32) {
		row_offset = ADDR_MAP_SHARED + (celly - 32) * 128;
	}
	int n = p8_memory[row_offset + cellx];

	lua_pushinteger(L, n);

	return 1;
}

int mset(lua_State* L) {
	int cellx = luaL_optnumber(L, 1, 0);
	int celly = luaL_optnumber(L, 2, 0);
	int snum = luaL_optinteger(L, 3, 0);

	int row_offset = ADDR_MAP + celly * 128;
	if (celly >= 32) {
		row_offset = ADDR_MAP_SHARED + (celly - 32) * 128;
	}
	p8_memory[row_offset + cellx] = snum;

	return 0;
}

int cursor(lua_State* L) {
	int original_x = p8_memory[ADDR_DS_CURSOR_X];
	int original_y = p8_memory[ADDR_DS_CURSOR_Y];
	int original_color = p8_memory[ADDR_DS_COLOR];

	int x = luaL_optinteger(L, 1, 0);
	int y = luaL_optinteger(L, 2, 0);
	int col = luaL_optinteger(L, 3, original_color);

	p8_memory[ADDR_DS_CURSOR_X] = x;
	p8_memory[ADDR_DS_CURSOR_Y] = y;
	p8_memory[ADDR_DS_COLOR] = col;

	lua_pushinteger(L, original_x);
	lua_pushinteger(L, original_y);
	lua_pushinteger(L, original_color);
	return 3;
}

int print(lua_State* L) {
	std::string text;
	if (!lua_isstring(L, 1)) {
		text = "[]";
	}
	else {
		text = luaL_checkstring(L, 1);
	}

	if (lua_isinteger(L, 4)) {
		int x = luaL_checknumber(L, 2);
		int y = luaL_checknumber(L, 3);
		int col = luaL_checkinteger(L, 4);

		p8_memory[ADDR_DS_COLOR] = col;
		font->print(text, x, y, false);
	}
	if (lua_isnumber(L, 3)) {
		int x = luaL_checknumber(L, 2);
		int y = luaL_checknumber(L, 3);

		font->print(text, x, y, false);
		return 0;
	}

	if (lua_isinteger(L, 2)) {
		int col = luaL_checkinteger(L, 2);
		p8_memory[ADDR_DS_COLOR] = col;
	}
	font->print(text, p8_memory[ADDR_DS_CURSOR_X], p8_memory[ADDR_DS_CURSOR_Y], true);

	return 0;
}

int color(lua_State* L) {
	int col = luaL_optinteger(L, 1, 6);
	p8_memory[ADDR_DS_COLOR] = col;
	return 0;
}

bool hexint_to_num(std::string &hex, short* dest) {
	*dest = 0;
	for (int i = 0; i < hex.size(); i++) {
		char c = hex[i];
		unsigned char val;
		if (c >= '0' && c <= '9') {
			val = c - '0';
		}
		else if (c >= 'a' && c <= 'f') {
			val = 10 + (c - 'a');
		}
		else {
			return false;
		}

		*dest = (*dest << 4) | val;
	}
	return true;
}
bool decint_to_num(std::string &dec, short* dest) {
	*dest = 0;
	for (int i = 0; i < dec.size(); i++) {
		char c = dec[i];
		if (c >= '0' && c <= '9') {
			*dest = (*dest * 10) + (c - '0');
		}
		else {
			return false;
		}
	}
	return true;
}
bool binint_to_num(std::string &bin, short* dest) {
	*dest = 0;
	for (int i = 0; i < bin.size(); i++) {
		char c = bin[i];
		if (c == '0' || c == '1') {
			*dest = (*dest << 1) + (c - '0');
		}
		else {
			return false;
		}
	}
	return true;
}
bool decimal_to_num(std::string &str, int base, double* dest) {
	bool (*fn)(std::string &, short *) = NULL;
	if (base == 2) {
		fn = &binint_to_num;
	} else if(base == 10) {
		fn = &decint_to_num;
	} else if (base == 16) {
		fn = &hexint_to_num;
	}
	else {
		return false;
	}

	int dec_pos = str.find(".");
	short integer_part = 0;
	std::string integer_string = str.substr(0, dec_pos);
	bool r = fn(integer_string, &integer_part);
	if (!r) {
		return false;
	}

	*dest = (double)integer_part;
	double factor = 1;
	for (int i = dec_pos + 1; i < str.size(); i++) {
		short value;
		std::string character = str.substr(i, 1);
		r = fn(character, &value);
		if (!r) {
			return false;
		}
		factor *= base;
		*dest += (double)value / factor;
	}

	return true;
}

// 0 = nil, 1 = int, 2 = dec
int string_to_num(std::string& str, short* shortval, double* doubleval) {
	if (str.find("0x") == 0) {
		str = str.replace(0, 2, "");

		if (str.find(".") == std::string::npos) {
			if (hexint_to_num(str, shortval)) {
				return 1;
			}
		}
		else {
			if (decimal_to_num(str, 16, doubleval)) {
				return 2;
			}
		}
		return 0;
	}
	if (str.find("0b") == 0) {
		if (str.find(".") == std::string::npos) {
			if (binint_to_num(str, shortval)) {
				return 1;
			}
		}
		else {
			if (decimal_to_num(str, 2, doubleval)) {
				return 2;
			}
		}
		return 0;
	}

	if (str.find(".") == std::string::npos) {
		if (decint_to_num(str, shortval)) {
			return 1;
		}
	}
	else {
		if (decimal_to_num(str, 10, doubleval)) {
			return 2;
		}
	}
	return 0;
}

int tonum(lua_State* L) {
	std::string str = luaL_checkstring(L, 1);
	double decimal_value;
	short integer_value;

	int result = string_to_num(str, &integer_value, &decimal_value);

	if (result == 1) {
		lua_pushinteger(L, integer_value);
	}
	else if (result == 2) {
		lua_pushnumber(L, decimal_value);
	}
	else {
		lua_pushnil(L);
	}

	return 1;
}

int pal(lua_State* L) {
	if (lua_gettop(L) == 0) {
		renderer->reset_draw_pal();
		renderer->reset_screen_pal();
		return 0;
	}

	if (lua_istable(L, 1)) {
		DEBUGLOG << "pal with table unsupported" << ENDL;

		return 0;
	}

	int c0 = luaL_checkinteger(L, 1);
	int c1 = luaL_optinteger(L, 2, 0);
	int p = luaL_optinteger(L, 3, 0);
	if (p > 1) {
		DEBUGLOG << "pal for pattern unsupported" << ENDL;
		return 0;
	}
	int addr = ADDR_DS_DRAW_PAL;
	if (p == 1) {
		addr = ADDR_DS_SCREEN_PAL;
	}

	p8_memory[addr + c0] = c1;

	return 0;
}

int palt(lua_State* L) {
	if (lua_gettop(L) == 0) {
		renderer->reset_transparency_pal();
		return 0;
	}

	if (lua_gettop(L) == 1) {
		int transp = luaL_checkinteger(L, 1);
		for (int c = 0; c < 16; c++) {
			int mask = 0x01 << (15 - c);
			renderer->set_color_transparent(c, (transp & c) > 0);
		}

		return 0;
	}

	int col = luaL_checkinteger(L, 1);
	int t = lua_toboolean(L, 2);

	renderer->set_color_transparent(col, t);

	return 0;
}

int camera(lua_State* L) {
	int original_x = memory_read_short(ADDR_DS_CAMERA_X);
	int original_y = memory_read_short(ADDR_DS_CAMERA_Y);

	int x = luaL_optnumber(L, 1, 0);
	int y = luaL_optnumber(L, 2, 0);

	memory_write_short(ADDR_DS_CAMERA_X, x);
	memory_write_short(ADDR_DS_CAMERA_Y, y);

	lua_pushinteger(L, original_x);
	lua_pushinteger(L, original_y);

	return 2;
}

int sqrt(lua_State* L) {
	float f = luaL_checknumber(L, 1);

	lua_pushnumber(L, sqrtf(f));

	return 1;
}

int sin(lua_State* L) {
	float f = luaL_checknumber(L, 1);

	lua_pushnumber(L, -sinf(f * 2 * M_PI));

	return 1;
}

int atan2(lua_State* L) {
	float dx = luaL_checknumber(L, 1);
	float dy = luaL_checknumber(L, 2);

	float result = atan2(-dy, dx) / (2 * M_PI);
	if (result < 0) {
		result += 1;
	}
	lua_pushnumber(L, result);

	return 1;
}

int cos(lua_State* L) {
	float f = luaL_checknumber(L, 1);

	lua_pushnumber(L, cosf(f * 2 * M_PI));

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
		DEBUGLOG << "extcmd: Unrecognized command " << cmd << ENDL;
	}

	return 0;
}

int fget(lua_State* L) {
	unsigned char sprite = luaL_checkinteger(L, 1);
	char flag = luaL_optinteger(L, 2, -1);

	unsigned char flags = p8_memory[ADDR_SPRITE_FLAGS + sprite];
	if (flag == -1) {
		lua_pushinteger(L, flags);
	}
	else {
		unsigned char bitmask = 1 << flag;
		lua_pushboolean(L, (flags & bitmask) > 0);
	}

	return 1;
}

int fset(lua_State* L) {
	unsigned char sprite = luaL_checkinteger(L, 1);

	if (lua_gettop(L) == 3) {
		unsigned char flag = luaL_checkinteger(L, 2);
		bool checked = lua_toboolean(L, 3);

		unsigned char bitmask = 1 << flag;
		if (checked) {
			p8_memory[ADDR_SPRITE_FLAGS + sprite] |= bitmask;
		} else {
			p8_memory[ADDR_SPRITE_FLAGS + sprite] &= ~bitmask;
		}
	}
	else {
		unsigned char value = luaL_checkinteger(L, 2);
		p8_memory[ADDR_SPRITE_FLAGS + sprite] = value;
	}

	return 0;
}

int dget(lua_State* L) {
	lua_pushinteger(L, 0);

	return 1;
}

int shr(lua_State* L) {
	int num = lua_tonumber(L, 1);
	int bits = lua_tonumber(L, 2);

	lua_pushinteger(L, num >> bits);

	return 1;
}

int shl(lua_State* L) {
	int num = lua_tonumber(L, 1);
	int bits = lua_tonumber(L, 2);

	lua_pushinteger(L, num << bits);

	return 1;
}

int printh(lua_State* L) {
	std::string str = luaL_checkstring(L, 1);

	DEBUGLOG << str << ENDL;

	return 0;
}

int sget(lua_State* L) {
	unsigned int x = lua_tointeger(L, 1);
	unsigned int y = lua_tointeger(L, 2);

	if (x > 127 || y > 127) {
		lua_pushinteger(L, 0);
	}
	else {
		unsigned char pair = p8_memory[ADDR_SPRITE_SHEET + (y * 128 + x) / 2];
		if (x % 2 == 0) {
			unsigned char left = pair & 0x0F;
			lua_pushinteger(L, left);
		}
		else {
			unsigned char right = pair >> 4;
			lua_pushinteger(L, right);
		}
	}

	return 1;
}

int sset(lua_State* L) {
	unsigned int x = lua_tointeger(L, 1);
	unsigned int y = lua_tointeger(L, 2);
	unsigned char color = luaL_optinteger(L, 3, p8_memory[ADDR_DS_COLOR]) & 0x0F;

	if (x > 127 || y > 127) {
		return 0;
	}

	int addr = ADDR_SPRITE_SHEET + (y * 128 + x) / 2;
	if (x % 2 == 0) {
		// Update left - Least significant bits.
		p8_memory[addr] = (p8_memory[addr] & 0xF0) | color;
	}
	else {
		// Update right - Most significant bits.
		p8_memory[addr] = (p8_memory[addr] & 0x0F) | (color << 4);
	}

	return 0;
}

int clip(lua_State* L) {
	int x = luaL_optnumber(L, 1, 0);
	int y = luaL_optnumber(L, 2, 0);
	int w = luaL_optnumber(L, 3, P8_WIDTH-1);
	int h = luaL_optnumber(L, 4, P8_HEIGHT-1);
	bool clip_previous = lua_toboolean(L, 5);

	int x_end = x + w - 1;
	int y_end = y + h - 1;

	int px = p8_memory[ADDR_DS_CLIP_RECT];
	int py = p8_memory[ADDR_DS_CLIP_RECT + 1];
	int px_end = p8_memory[ADDR_DS_CLIP_RECT + 2];
	int py_end = p8_memory[ADDR_DS_CLIP_RECT + 3];
	int pw = px_end - px + 1;
	int ph = py_end - py + 1;

	x = std::max(0, std::min(x, P8_WIDTH - 1));
	y = std::max(0, std::min(y, P8_WIDTH - 1));
	x_end = std::max(0, std::min(x_end, P8_WIDTH - 1));
	y_end = std::max(0, std::min(y_end, P8_WIDTH - 1));

	if (clip_previous) {
		x = std::max(x, px);
		y = std::max(y, py);
		x_end = std::min(x_end, px_end);
		y_end = std::min(y_end, py_end);
	}

	p8_memory[ADDR_DS_CLIP_RECT] = x;
	p8_memory[ADDR_DS_CLIP_RECT + 1] = y;
	p8_memory[ADDR_DS_CLIP_RECT + 2] = x_end;
	p8_memory[ADDR_DS_CLIP_RECT + 3] = y_end;

	lua_pushinteger(L, px);
	lua_pushinteger(L, py);
	lua_pushinteger(L, pw);
	lua_pushinteger(L, ph);
	return 4;
}

int noop(lua_State* L) {
	return 0;
}