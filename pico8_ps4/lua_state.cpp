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

#define DEBUGLOG LuaState_DEBUGLOG
Log DEBUGLOG = logger.log("LuaState");

int rnd(lua_State* L);
int flr(lua_State* L);
int srand(lua_State* L);
int tostr(lua_State* L);
int sub(lua_State* L);
int btn(lua_State* L);
int btnp(lua_State* L);
int cls(lua_State* L);
int spr(lua_State* L);
int cursor(lua_State* L);
int print(lua_State* L);
int color(lua_State* L);
int tonum(lua_State* L);
int add(lua_State* L);
int deli(lua_State* L);
int rectfill(lua_State* L);
int noop(lua_State* L);
int time(lua_State* L);
int type(lua_State* L);
int rect(lua_State* L);
int sfx(lua_State* L);
int poke(lua_State* L);
int pal(lua_State* L);
int palt(lua_State* L);
int camera(lua_State* L);
int sqrt(lua_State* L);

LuaState::LuaState()
{
	srand(std::time(NULL));

    this->state = luaL_newstate();
	this->is60FPS = false;

	lua_pushcfunction(this->state, rnd);
	lua_setglobal(this->state, "rnd");
	lua_pushcfunction(this->state, flr);
	lua_setglobal(this->state, "flr");
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
	lua_pushcfunction(this->state, rectfill);
	lua_setglobal(this->state, "rectfill");
	lua_pushcfunction(this->state, sfx);
	lua_setglobal(this->state, "sfx");
	lua_pushcfunction(this->state, time);
	lua_setglobal(this->state, "time");
	lua_pushcfunction(this->state, type);
	lua_setglobal(this->state, "type");
	lua_pushcfunction(this->state, rect);
	lua_setglobal(this->state, "rect");
	lua_pushcfunction(this->state, poke);
	lua_setglobal(this->state, "poke");
	lua_pushcfunction(this->state, noop);
	lua_setglobal(this->state, "music");
	lua_pushcfunction(this->state, pal);
	lua_setglobal(this->state, "pal");
	lua_pushcfunction(this->state, palt);
	lua_setglobal(this->state, "palt");
	lua_pushcfunction(this->state, camera);
	lua_setglobal(this->state, "camera");
	lua_pushcfunction(this->state, sqrt);
	lua_setglobal(this->state, "sqrt");

	std::string all =
		"function all(t) \
			local i = 0 \
			local n = #t \
			return function() \
				i = i + 1 \
				if i <= n then return t[i] end \
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
}

int sfx(lua_State* L) {
	int n = luaL_checkinteger(L, 1);
	int channel = luaL_optinteger(L, 2, -1);
	int offset = luaL_optinteger(L, 3, 0);
	int length = luaL_optinteger(L, 4, 31);

	audioManager->playSfx(n, channel, offset, length);

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

int rectfill(lua_State* L) {
	int x0 = luaL_checkinteger(L, 1);
	int y0 = luaL_checkinteger(L, 2);
	int x1 = luaL_checkinteger(L, 3);
	int y1 = luaL_checkinteger(L, 4);
	int col = luaL_optnumber(L, 5, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_rectangle(x0, y0, x1, y1, true);

	return 0;
}

int rect(lua_State* L) {
	int x0 = luaL_checkinteger(L, 1);
	int y0 = luaL_checkinteger(L, 2);
	int x1 = luaL_checkinteger(L, 3);
	int y1 = luaL_checkinteger(L, 4);
	int col = luaL_optnumber(L, 5, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_rectangle(x0, y0, x1, y1, false);

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
		lua_pop(this->state, 1);
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
	unsigned char addr = luaL_checkinteger(L, 1) & 0x0FF;
	unsigned char value = luaL_optinteger(L, 2, 0) & 0x0FF;
	DEBUGLOG << "Poked " << addr << ENDL;
	p8_memory[addr] = value;

	for (int i = 3; lua_isinteger(L, i); i++) {
		value = lua_tointeger(L, 2) & 0x0FF;
		addr++;
		DEBUGLOG << "Poked " << addr << ENDL;
		p8_memory[addr] = value;
	}

	return 0;
}

int rnd(lua_State* L) {
	double max = luaL_optnumber(L, 1, 1);
	double result = std::rand() * max / RAND_MAX;
	lua_pushnumber(L, result);
	return 1;
}

int flr(lua_State* L) {
	double num = luaL_checknumber(L, 1);
	lua_pushinteger(L, floor(num));
	return 1;
}

int srand(lua_State* L) {
	double num = luaL_checknumber(L, 1);
	std::srand(num);
	return 0;
}

// TODO https://pico-8.fandom.com/wiki/Tostr decimal hex?
int tostr(lua_State* L) {
	double num = luaL_checknumber(L, 1);
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
	if (lua_isinteger(L, 1)) {
		int i = luaL_checkinteger(L, 1);
		int p = luaL_optinteger(L, 2, 0);

		lua_pushboolean(L, machineState->wasButtonPressed(p, (P8_Key)i));
	}
	else {
		lua_pushinteger(L, machineState->getButtonsState());
	}
	return 1;
}

int btn(lua_State* L) {
	if (lua_isinteger(L, 1)) {
		int i = luaL_checkinteger(L, 1);
		int p = luaL_optinteger(L, 2, 0);

		lua_pushboolean(L, machineState->isButtonPressed(p, (P8_Key)i));
	}
	else {
		lua_pushinteger(L, machineState->getButtonsState());
	}
	return 1;
}

int cls(lua_State* L) {
	int col = luaL_optinteger(L, 3, 0);

	renderer->clear_screen(col);
	p8_memory[ADDR_DS_CURSOR_X] = 0;
	p8_memory[ADDR_DS_CURSOR_Y] = 0;

	return 0;
}

// TODO flip
int spr(lua_State* L) {
	int n = luaL_checkinteger(L, 1);
	int x = luaL_checknumber(L, 2);
	int y = luaL_checknumber(L, 3);
	int w = luaL_optnumber(L, 4, 1.0) * 8;
	int h = luaL_optnumber(L, 5, 1.0) * 8;

	renderer->draw_sprite(n, x, y, w, h);

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

// TODO decimal hex
int tonum(lua_State* L) {
	std::string str = luaL_checkstring(L, 1);

	std::stringstream ss;
	if (str.find("0x") == 0) {
		str = str.replace(0, 2, "");
		ss << std::hex << str;
		int out;
		ss >> out;
		lua_pushinteger(L, out);
	}
	else {
		ss << str;
		double out;
		ss >> out;
		if (str.find(".") != std::string::npos) {
			lua_pushnumber(L, out);
		}
		else {
			lua_pushinteger(L, out);

		}
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
	int c1 = luaL_checkinteger(L, 2);
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
	int original_x = p8_memory[ADDR_DS_CAMERA_X];
	int original_y = p8_memory[ADDR_DS_CAMERA_Y];

	int x = luaL_optinteger(L, 1, 0);
	int y = luaL_optinteger(L, 2, 0);

	p8_memory[ADDR_DS_CAMERA_X] = x;
	p8_memory[ADDR_DS_CAMERA_Y] = y;

	lua_pushinteger(L, original_x);
	lua_pushinteger(L, original_y);

	return 2;
}

int sqrt(lua_State* L) {
	float f = luaL_checknumber(L, 0);

	lua_pushnumber(L, std::sqrtf(f));

	return 1;
}

int noop(lua_State* L) {
	return 0;
}