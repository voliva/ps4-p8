/*
* Test file for p8lua -> lua conversion, as it breaks easily
*/

#include <string>
#include <iostream>
#include <vector>

using namespace std;

/// Broken
// Omega zone
// DominionEx bullets

bool run_test(string name, string p8lua, vector<char>& expected);

typedef struct {
	string name;
	string p8lua;
	vector<char> expected;
} Test;

Test tests[] = {
	Test{
		"Transforms assignment operators correctly when there's a comment on the same line",
		R"V0G0N(
			function test()
				x=0;
				x+=3--5
				return x
			end
		)V0G0N",
		{3}
	},
	Test{
		"Doesn't strip strings that contain --",
		R"V0G0N(
			function test()
				local value="hello -- bye"
			end
		)V0G0N",
		{}
	},
	Test{
		"Works with multiline comments",
		R"V0G0N(
			--[[ multiline
			comment
			-- end]]
			function test()
			end
		)V0G0N",
		{}
	},
	Test{
		"Understands integer division",
		R"V0G0N(
			function test()
				x=8\5;
				return x
			end
		)V0G0N",
		{1}
	},
	Test{
		"Replaces btn and btnp buttons to their number",
		"function test() \
			btn(\139) \
			btnp(\151) \
		end",
		{}
	},
	Test{
		"Replaces fillp pattern literals to their number",
		"function test() \
			btnp(\141) \
		end",
		{}
	},
	Test{
		"Print shorthand",
		R"V0G0N(
			function test()
				?"hello"
			end
		)V0G0N",
		{}
	},
	Test{
		"Inequality",
		R"V0G0N(
			function test()
				if 3 != 2 then return 1 else return 0 end
			end
		)V0G0N",
		{1}
	},
	Test{
		"Binary literals",
		R"V0G0N(
			function test()
				return 0b01101010
			end
		)V0G0N",
		{0x6A}
	},
	Test{
		"Unary operators",
		R"V0G0N(
			function test()
				return
					@123,
					%123,
					$123,
					~123
			end
		)V0G0N",
		{2,3,4,5}
	},
	Test{
		"Binary operators",
		R"V0G0N(
			function test()
				return
					1 | 2,
					1 & 2,
					1 ^^ 2,
					1 >>> 2,
					1 <<> 2,
					1 >>< 2,
					1 << 2,
					1 >> 2
			end
		)V0G0N",
		{6,7,8,9,10,11,12,13}
	},
	Test{
		"Assignment operators",
		R"V0G0N(
			function test()
				local x=0
				x+=1
				x *= 3
				x <<= 1
				x >>= 1
				x <<>= 1
				x >><= 1
				x >>>= 1
				return x
			end
		)V0G0N",
		{9}
	},
	Test{
		"Assignment operators",
		R"V0G0N(
			function test()
				local x=0
				x += 1 x *= 3
				return x
			end
		)V0G0N",
		{3}
	},
	Test{
		"If shorthand",
		R"V0G0N(
			function test()
				local x=0
				if (x < 3) return 3
				return x
			end
		)V0G0N",
		{3}
	},
	Test{
		"Values which call iif",
		R"V0G0N(
			function iif() end
			function test()
				iif(123)
			end
		)V0G0N",
		{}
	},
	Test{
		"Multiline if",
		R"V0G0N(
			function test()
				if (true) and
					(true) or
					(false) then
					return 1
				end
			end
		)V0G0N",
		{1}
	},
	Test{
		"If with and/or inside",
		R"V0G0N(
			function test()
				if (true) fakeFn(0 and 10 or 9)
			end
		)V0G0N",
		{}
	},
	Test{
		"Whitespace omission on if",
		R"V0G0N(
			function test()
				local x=0
				if x < 3then return 3 end
				return x
			end
		)V0G0N",
		{3}
	},
	Test{
		"Whitespace omission on for",
		R"V0G0N(
			function test()
				local x=0
				for i=1,5do x=x+1 end
				return x
			end
		)V0G0N",
		{5}
	}
	// If shorthand tests
	// Space omit on if + for
};

int main() {
	int n_tests = sizeof(tests) / sizeof(Test);
	for (int i = 0; i < n_tests; i++) {
		run_test(tests[i].name, tests[i].p8lua, tests[i].expected);
	}
	
	cout << "finished" << endl;

	return 0;
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
	string btn_fns =
		"function btn(value) return value end \
		function btnp(value) return value end \
		function fillp(value) return value end \
		function print() return 1 end \
		function peek() return 2 end \
		function peek2() return 3 end \
		function peek4() return 4 end \
		function bnot() return 5 end \
		function bor() return 6 end \
		function band() return 7 end \
		function bxor() return 8 end \
		function lshr() return 9 end \
		function rotl() return 10 end \
		function rotr() return 11 end \
		function shl() return 12 end \
		function shr() return 13 end";
	luaL_loadbuffer(l, btn_fns.c_str(), btn_fns.length(), "btn_fns");
	lua_pcall(l, 0, 0, 0);

	string stdLua = p8lua_to_std_lua(p8lua);

	int error = luaL_loadbuffer(l, stdLua.c_str(), stdLua.length(), "program") || lua_pcall(l, 0, 0, 0);

	if (error) {
		std::string e = lua_tostring(l, -1);
		lua_pop(l, 1);
		cout << name << ": " << e << endl;
		cout << stdLua << endl;

		lua_close(l);
		return false;
	}

	lua_getglobal(l, "test");
	if (lua_pcall(l, 0, expected.size(), 0)) {
		std::string e = lua_tostring(l, -1);
		lua_pop(l, 1);
		cout << name << ": " << e << endl;
		cout << stdLua << endl;

		lua_close(l);
		return false;
	}

	bool result = true;

	for (int i = 0; i < expected.size(); i++) {
		char v = (char)lua_tointeger(l, i-expected.size());
		if (v != expected[i]) {
			cout << name << " element " << i << ": Expected " << (int)expected[i] << " but got " << (int)v << endl;
			result = false;
		}
	}

	if(!result) {
		cout << stdLua << endl;
	}

	lua_close(l);
	return result;
}