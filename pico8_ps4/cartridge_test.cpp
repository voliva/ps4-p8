/*
* Test file for p8lua -> lua conversion, as it breaks easily
*/

#include <string>
#include <iostream>
#include <vector>

using namespace std;

bool run_test(string name, string p8lua, vector<char>& expected);

typedef struct {
	string name;
	string p8lua;
	vector<char> expected;
} Test;

Test tests[] = { Test{
	"test of the test",
	"function test() return 1 end",
	{1}
} };

void main() {
	int n_tests = sizeof(tests) / sizeof(Test);
	for (int i = 0; i < n_tests; i++) {
		run_test(tests[i].name, tests[i].p8lua, tests[i].expected);
	}
}

extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}
extern string p8lua_to_std_lua(string& s);
bool run_test(string name, string p8lua, vector<char> &expected) {
	lua_State *l = luaL_newstate();

	string fakeFn =
		"function fakeFn(value) \
			return value \
		end";
	luaL_loadbuffer(l, fakeFn.c_str(), fakeFn.length(), "fakeFn");
	lua_pcall(l, 0, 0, 0);

	string stdLua = p8lua_to_std_lua(p8lua);

	int error = luaL_loadbuffer(l, stdLua.c_str(), stdLua.length(), "program") || lua_pcall(l, 0, 0, 0);

	if (error) {
		std::string e = lua_tostring(l, -1);
		lua_pop(l, 1);
		cout << name << ": " << e << endl;

		lua_close(l);
		return false;
	}

	lua_getglobal(l, "test");
	if (lua_pcall(l, 0, 0, 0)) {
		std::string e = lua_tostring(l, -1);
		lua_pop(l, 1);
		cout << name << ": " << e << endl;

		lua_close(l);
		return false;
	}

	bool result = true;
	int len = lua_gettop(l);
	if (len != expected.size()) {
		cout << "Different expected length: Expected " << expected.size() << " but got " << len << endl;
		result = false;
	}

	int m = max(len, (int)expected.size());
	for (int i = 0; i < m; i++) {
		char v = (char)lua_tointeger(l, i + 1);
		if (v != expected[i]) {
			cout << "element " << i << ": Expected " << expected[i] << " but got " << v << endl;
			result = false;
		}
	}

	for (int i = m; i < len; i++) {
		cout << "element " << i << ": Extra " << (char)lua_tointeger(l, i + 1) << endl;
	}

	lua_close(l);
	return result;
}