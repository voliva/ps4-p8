#include "lua_state.h"
#include "log.h"
#include <math.h>
#include <stdlib.h>
#include <sstream>
#include "renderer.h"
#include "machine_state.h"

#define DEBUGLOG LuaState_DEBUGLOG
Log DEBUGLOG = logger.log("LuaState");

int rnd(lua_State* L);
int flr(lua_State* L);
int srand(lua_State* L);
int tostr(lua_State* L);
int sub(lua_State* L);
int btnp(lua_State* L);
int cls(lua_State* L);
int spr(lua_State* L);
int cursor(lua_State* L);
int print(lua_State* L);
int color(lua_State* L);
int tonum(lua_State* L);
int add(lua_State* L);
int rectfill(lua_State* L);
int noop(lua_State* L);

LuaState::LuaState()
{
	srand(time(NULL));

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
	lua_pushcfunction(this->state, noop);
	lua_setglobal(this->state, "menuitem");
	lua_pushcfunction(this->state, rectfill);
	lua_setglobal(this->state, "rectfill");
	lua_pushcfunction(this->state, noop);
	lua_setglobal(this->state, "sfx");

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

int rectfill(lua_State* L) {
	int x0 = luaL_checkinteger(L, 1);
	int y0 = luaL_checkinteger(L, 2);
	int x1 = luaL_checkinteger(L, 3);
	int y1 = luaL_checkinteger(L, 4);
	int col = luaL_optnumber(L, 5, machineState->getColor());

	machineState->setColor(col);
	SDL_Rect rect{
		x0, y0, x1 - x0 + 1, y1 - y0 + 1
	};
	SDL_RenderFillRect(renderer, &rect);

	return 0;
}

LuaState::~LuaState()
{
	lua_close(this->state);
}

bool LuaState::loadProgram(std::string& program)
{
    int error = luaL_loadbuffer(this->state, program.c_str(), program.length(), "program") || lua_pcall(this->state, 0, 0, 0);

	if (error) {
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

int rnd(lua_State* L) {
	double max = luaL_optnumber(L, 1, 1);
	double result = std::rand() * max / RAND_MAX;
	lua_pushnumber(L, result);
	return 1;
}

int flr(lua_State* L) {
	double num = luaL_checknumber(L, 1);
	lua_pushnumber(L, floor(num));
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

int cls(lua_State* L) {
	int original_color = machineState->getColor();
	int col = luaL_optinteger(L, 3, 0);
	machineState->setColor(col);
	clear_screen();
	machineState->setColor(original_color);

	machineState->cursor.x = 0;
	machineState->cursor.y = 0;
	return 0;
}

// TODO flip
int spr(lua_State* L) {
	int n = luaL_checkinteger(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	int w = luaL_optnumber(L, 4, 1.0) * 8;
	int h = luaL_optnumber(L, 5, 1.0) * 8;

	draw_sprite(n, x, y, w, h);

	return 0;
}

int cursor(lua_State* L) {
	int original_color = machineState->getColor();
	SDL_Point original_cursor = machineState->cursor; // TODO is it getting copied?

	int x = luaL_optinteger(L, 1, 0);
	int y = luaL_optinteger(L, 2, 0);
	int col = luaL_optinteger(L, 3, original_color);

	machineState->cursor.x = x;
	machineState->cursor.y = y;
	machineState->setColor(col);

	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	lua_pushinteger(L, col);
	return 3;
}

int print(lua_State* L) {
	std::string text = luaL_checkstring(L, 1);
	if (lua_isinteger(L, 4)) {
		int x = luaL_checkinteger(L, 2);
		int y = luaL_checkinteger(L, 3);
		int col = luaL_checkinteger(L, 4);

		machineState->setColor(col);
		machineState->print(text, SDL_Point{ x, y });
		return 0;
	}
	if (lua_isinteger(L, 3)) {
		int x = luaL_checkinteger(L, 2);
		int y = luaL_checkinteger(L, 3);

		machineState->print(text, SDL_Point { x, y });
		return 0;
	}

	if (lua_isinteger(L, 2)) {
		int col = luaL_checkinteger(L, 2);
		machineState->setColor(col);
	}
	machineState->print(text);

	return 0;
}

int color(lua_State* L) {
	int col = luaL_optinteger(L, 1, 6);
	machineState->setColor(col);
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
		lua_pushnumber(L, out);
	}
	else {
		ss << str;
		double out;
		ss >> out;

		lua_pushnumber(L, out);
	}

	return 1;
}

int noop(lua_State* L) {
	return 0;
}