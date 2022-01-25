#include "lua_fns.h"
#include "machine_state.h"
#include <math.h>

int rnd(lua_State* L) {
	fix16_t max = 0;
	if (lua_isstring(L, 1)) {
		max = lua_tonumber(L, 1);
	}
	else {
		max = luaL_optnumber(L, 1, fix16_one);
	}
	unsigned short high_max = max >> 16;;
	unsigned short low_max = max & 0x0FFFF;

	unsigned int rnd = machineState->getRnd();
	unsigned short high_rnd = rnd >> 16;
	unsigned short low_rnd = rnd & 0x0FFFF;

	short high = high_rnd;
	if (high_max > 0) {
		high = high_rnd % high_max;
	}
	else if (low_max > 0) {
		high = 0;
	}

	unsigned short low = low_rnd;
	if (low_max > 0) {
		low = low_rnd % low_max;
	}

	fix16_t result = high << 16 | low;

	lua_pushnumber(L, result);
	return 1;
}

int srand(lua_State* L) {
	fix16_t num = luaL_checknumber(L, 1);
	machineState->setRndSeed(num);
	return 0;
}

int flr(lua_State* L) {
	int num = lua_tointeger(L, 1);
	lua_pushinteger(L, num);
	return 1;
}

int ceil(lua_State* L) {
	fix16_t num = luaL_checknumber(L, 1);
	lua_pushinteger(L, fix16_to_int(fix16_ceil(num)));
	return 1;
}

int sqrt(lua_State* L) {
	fix16_t f = luaL_checknumber(L, 1);

	lua_pushnumber(L, fix16_sqrt(f));

	return 1;
}

int sin(lua_State* L) {
	fix16_t f = luaL_checknumber(L, 1);

	lua_pushnumber(L, -fix16_sin(
		fix16_mul(f, fix16_pi << 1) // fix16_pi << 1 = 2*PI with fix16 format
	));

	return 1;
}

int atan2(lua_State* L) {
	fix16_t dx = luaL_checknumber(L, 1);
	fix16_t dy = luaL_checknumber(L, 2);

	fix16_t result = fix16_div(fix16_atan2(-dy, dx), fix16_pi << 1);
	if (result < 0) {
		result += fix16_one;
	}
	lua_pushnumber(L, result);

	return 1;
}

int cos(lua_State* L) {
	fix16_t f = luaL_checknumber(L, 1);

	lua_pushnumber(L, fix16_cos(
		fix16_mul(f, fix16_pi << 1)
	));

	return 1;
}

int shr(lua_State* L) {
	fix16_t num = lua_tonumber(L, 1);
	int bits = lua_tointeger(L, 2);

	lua_pushnumber(L, num >> bits);

	return 1;
}

int shl(lua_State* L) {
	fix16_t num = lua_tonumber(L, 1);
	int bits = lua_tointeger(L, 2);

	lua_pushnumber(L, num << bits);

	return 1;
}

int band(lua_State* L) {
	fix16_t a = lua_tonumber(L, 1);
	fix16_t b = lua_tonumber(L, 2);

	lua_pushnumber(L, a & b);

	return 1;
}

int bor(lua_State* L) {
	fix16_t a = lua_tonumber(L, 1);
	fix16_t b = lua_tonumber(L, 2);

	lua_pushnumber(L, a | b);

	return 1;
}

void load_math_fns(lua_State* L)
{
	register_fn(L, "__rnd_num", rnd);
	register_fn(L, "flr", flr);
	register_fn(L, "ceil", ceil);
	register_fn(L, "srand", srand);
	register_fn(L, "sqrt", sqrt);
	register_fn(L, "cos", cos);
	register_fn(L, "sin", sin);
	register_fn(L, "atan2", atan2);
	register_fn(L, "shr", shr);
	register_fn(L, "shl", shl);
	register_fn(L, "band", band);
	register_fn(L, "bor", bor);

	register_lua_fn(L, "min", R"V0G0N(
function min(first, second)
	second = second or 0
	if first < second then
		return first
	else
		return second
	end
end
	)V0G0N");


	register_lua_fn(L, "max", R"V0G0N(
function max(first, second)
	second = second or 0
	if first > second then
		return first
	else
		return second
	end
end
	)V0G0N");


	register_lua_fn(L, "mid", R"V0G0N(
function mid(a, b, c)
	if b <= a and a <= c then return a end
	if c <= a and a <= b then return a end
	if a <= b and b <= c then return b end
	if c <= b and b <= a then return b end
	return c
end
	)V0G0N");


	register_lua_fn(L, "abs", R"V0G0N(
function abs(a)
	if a < 0 then return -a end
	return a
end
	)V0G0N");


	register_lua_fn(L, "sgn", R"V0G0N(
function sgn(value)
	if value >= 0 then return 1 else return -1 end
end
	)V0G0N");


	register_lua_fn(L, "rnd_lua", R"V0G0N(
function rnd(value)
	if type(value) == "table" then
		return value[__rnd_num(1 + flr(#value))]
	else
		return __rnd_num(value)
	end
end
	)V0G0N");
}