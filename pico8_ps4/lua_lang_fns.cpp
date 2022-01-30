#include "lua_fns.h"
#include <set>
#include <sstream>
#include "log.h"

Log DEBUGLOG = logger.log("LuaFns");

// Global FNs
std::set<std::string> alerted;
void alert_todo(std::string key) {
	if (alerted.count(key) == 0) {
		DEBUGLOG << "TODO called: " << key << ENDL;
		alerted.insert(key);
	}
}

void register_fn(lua_State* L, std::string name, lua_CFunction fn)
{
	lua_pushcfunction(L, fn);
	lua_setglobal(L, name.c_str());
}

void register_lua_fn(lua_State* L, std::string name, std::string code)
{
	luaL_loadbuffer(L, code.c_str(), code.length(), name.c_str());
	int error = lua_pcall(L, 0, 0, 0);

	if (error) {
		std::string e = lua_tostring(L, -1);
		lua_pop(L, 1);  // pop error message from the stack *
		DEBUGLOG << code << ENDL
			<< e << ENDL;
	}
}

int tostr(lua_State* L) {
	fix16_t num = lua_tonumber(L, 1);
	bool useHex = false;
	if (lua_isboolean(L, 2)) {
		useHex = lua_toboolean(L, 2);
	}

	std::ostringstream buf;
	if (useHex) {
		buf << std::hex << num;
		std::string raw_text = buf.str();
		// Pad left
		while (raw_text.length() < 8) {
			raw_text = "0" + raw_text;
		}

		std::string result = "0x" + raw_text.substr(0, 4) + "." + raw_text.substr(4);
		lua_pushstring(L, result.c_str());
	}
	else {
		buf << fix16_to_float(num);
		lua_pushstring(L, buf.str().c_str());
	}

	return 1;
}

bool hexint_to_num(std::string& hex, short* dest) {
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
bool decint_to_num(std::string& dec, short* dest) {
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
bool binint_to_num(std::string& bin, short* dest) {
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
bool decimal_to_num(std::string& str, int base, fix16_t* dest) {
	bool (*fn)(std::string&, short*) = NULL;
	if (base == 2) {
		fn = &binint_to_num;
	}
	else if (base == 10) {
		fn = &decint_to_num;
	}
	else if (base == 16) {
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

	double d_decimal_part = 0;
	double factor = 1;
	for (int i = dec_pos + 1; i < str.size(); i++) {
		short value;
		std::string character = str.substr(i, 1);
		r = fn(character, &value);
		if (!r) {
			return false;
		}
		factor *= base;
		d_decimal_part += (double)value / factor;
	}

	*dest = fix16_from_dbl(d_decimal_part);
	*dest = *dest | (integer_part << 16);

	return true;
}

// 0 = nil, 1 = int, 2 = dec
int string_to_num(std::string& str, short* shortval, fix16_t* doubleval) {
	bool neg = false;
	if (str[0] == '-') {
		neg = true;
		str = str.substr(1);
	}

	if (str.find("0x") == 0) {
		str = str.replace(0, 2, "");

		if (str.find(".") == std::string::npos) {
			if (hexint_to_num(str, shortval)) {
				if (neg) *shortval = -*shortval;
				return 1;
			}
		}
		else {
			if (decimal_to_num(str, 16, doubleval)) {
				if (neg) *doubleval = -*doubleval;
				return 2;
			}
		}
		return 0;
	}
	if (str.find("0b") == 0) {
		if (str.find(".") == std::string::npos) {
			if (binint_to_num(str, shortval)) {
				if (neg) *shortval = -*shortval;
				return 1;
			}
		}
		else {
			if (decimal_to_num(str, 2, doubleval)) {
				if (neg) *doubleval = -*doubleval;
				return 2;
			}
		}
		return 0;
	}

	if (str.find(".") == std::string::npos) {
		if (decint_to_num(str, shortval)) {
			if (neg) *shortval = -*shortval;
			return 1;
		}
	}
	else {
		if (decimal_to_num(str, 10, doubleval)) {
			if (neg) *doubleval = -*doubleval;
			return 2;
		}
	}
	return 0;
}

static const std::string WHITESPACE = " \n\r\t\f\v";
std::string ltrim(const std::string& s)
{
	size_t start = s.find_first_not_of(WHITESPACE);
	return (start == std::string::npos) ? "" : s.substr(start);
}
std::string rtrim(const std::string& s)
{
	size_t end = s.find_last_not_of(WHITESPACE);
	return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}
std::string trim(const std::string& s) {
	return rtrim(ltrim(s));
}

int tonum(lua_State* L) {
	std::string str = luaL_checkstring(L, 1);
	fix16_t decimal_value;
	short integer_value;

	str = trim(str);

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

int printh(lua_State* L) {
	std::string str = luaL_checkstring(L, 1);

	DEBUGLOG << str << ENDL;

	return 0;
}

int ord(lua_State* L) {
	std::string str = luaL_checkstring(L, 1);
	int idx = luaL_optinteger(L, 2, 1) - 1;

	lua_pushinteger(L, str[idx]);

	return 1;
}

int chr(lua_State* L) {
	int len = lua_gettop(L);
	std::vector<char> str(len);
	for (int i = 0; i < len; i++) {
		str[i] = lua_tointeger(L, i + 1);
	}

	std::string result(str.begin(), str.end());
	lua_pushstring(L, result.c_str());

	return 0;
}

void load_lang_fns(lua_State* L)
{
	register_fn(L, "printh", printh);
	register_fn(L, "tostr", tostr);
	register_fn(L, "tonum", tonum);
	register_fn(L, "ord", ord);
	register_fn(L, "chr", chr);

	register_lua_fn(L, "add", R"V0G0N(
function add(tbl, val, idx)
	if idx != nil then
		__lua_table.insert(tbl, idx, val)
	else
		__lua_table.insert(tbl, val);
	end
end
	)V0G0N");

	register_lua_fn(L, "deli", R"V0G0N(
function deli(tbl, idx)
	return __lua_table.remove(tbl, idx)
end
	)V0G0N");

	register_lua_fn(L, "del", R"V0G0N(
function del(table, value)
	for i=1,#table do
		if table[i] == value then
			return deli(table, i)
		end
	end
end
	)V0G0N");

	register_lua_fn(L, "unpack", R"V0G0N(
function unpack(tbl, i, j)
	return __lua_table.unpack(tbl, i, j)
end
	)V0G0N");

	register_lua_fn(L, "sub", R"V0G0N(
function sub(str, pos0, pos1)
	return __lua_string.sub(str, pos0, pos1)
end
	)V0G0N");

	register_lua_fn(L, "split", R"V0G0N(
function split(str, separator, convert_numbers)
	if convert_numbers == nil then
		convert_numbers = true
	end
	if type(str) == "number" then
		return {str}
	end
	if type(str) != "string" then
		return nil
	end
	separator = separator or ","
	convert_numbers = convert_numbers ~= false
	local result = {}
	if separator == "" then
		for i=1,#str do
			add(result, sub(str, i, i))
		end
	else
		local current = ""
		for i=1,#str do
			local c = sub(str, i, i)
			if c == separator then
				add(result, current)
				current = ""
			else
				current = current..c
			end
		end
		add(result, current)
	end
	if convert_numbers then
		for i=1,#result do
			local value = tonum(result[i])
			if value ~= nil then
				result[i] = value
			end
		end
	end
	return result
end
	)V0G0N");

	register_lua_fn(L, "count", R"V0G0N(
function count(table, value)
	local t=0
	for i=1,#table do
		if value == nil and table[i] ~= nil then
			t = t + 1
		elseif table[i] == value then
			t = t + 1
		end
	end
	return t
end
	)V0G0N");

	register_lua_fn(L, "foreach", R"V0G0N(
function foreach(tbl, fn)
	for v in all(tbl) do
		fn(v)
	end
end
	)V0G0N");

	// It needs to count from the end of the table in case the elements get removed in-between
	register_lua_fn(L, "all", R"V0G0N(
function all(t)
	if t == nil then return function() end end
	local n = #t
	return function()
		local v = nil
		while n >= 0 and v == nil do
			v = t[#t-n]
			n = n - 1
		end
		return v
	end
end
	)V0G0N");
}