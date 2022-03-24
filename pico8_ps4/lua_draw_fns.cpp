#include "lua_fns.h"
#include "memory.h"
#include "renderer.h"
#include "font.h"

int pset(lua_State* L) {
	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	int col = luaL_optinteger(L, 3, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_point(x, y);

	return 0;
}

int pget(lua_State* L) {
	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);

	lua_pushinteger(L, renderer->get_pixel(x, y));

	return 1;
}

int fillp(lua_State* L) {
	fix16_t pat = luaL_optnumber(L, 1, 0);
	memory_write_short(ADDR_DS_FILL_PAT, pat >> 16);
	p8_memory[ADDR_DS_FILL_PAT + 2] = (pat & 0x0FFFF) >> 8;

	return 0;
}

int line(lua_State* L) {
	int x0 = lua_tointeger(L, 1);
	int y0 = lua_tointeger(L, 2);
	int x1 = lua_tointeger(L, 3);
	int y1 = lua_tointeger(L, 4);
	int col = luaL_optinteger(L, 5, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_line(x0, y0, x1, y1);

	return 0;
}

int tline(lua_State* L) {
	int x0 = lua_tointeger(L, 1);
	int y0 = lua_tointeger(L, 2);
	int x1 = lua_tointeger(L, 3);
	int y1 = lua_tointeger(L, 4);
	fix16_t mx = lua_tonumber(L, 5);
	fix16_t my = lua_tonumber(L, 6);
	fix16_t mdx = luaL_optnumber(L, 7, 0x2000); // 0x2000 is 1/8 in fix16
	fix16_t mdy = luaL_optnumber(L, 8, 0);

	renderer->draw_textured_line(x0, y0, x1, y1, fix16_to_float(mx), fix16_to_float(my), fix16_to_float(mdx), fix16_to_float(mdy));

	return 0;
}

int rectfill(lua_State* L) {
	int x0 = lua_tointeger(L, 1);
	int y0 = lua_tointeger(L, 2);
	int x1 = lua_tointeger(L, 3);
	int y1 = lua_tointeger(L, 4);
	int col = luaL_optinteger(L, 5, p8_memory[ADDR_DS_COLOR]);

	// DEBUGLOG << x0 << " " << y0 << " " << x1 << " " << y1 << " " << col << ENDL;

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_rectangle(x0, y0, x1, y1, true);

	return 0;
}

int rect(lua_State* L) {
	int x0 = lua_tointeger(L, 1);
	int y0 = lua_tointeger(L, 2);
	int x1 = lua_tointeger(L, 3);
	int y1 = lua_tointeger(L, 4);
	int col = luaL_optinteger(L, 5, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_rectangle(x0, y0, x1, y1, false);

	return 0;
}

int ovalfill(lua_State* L) {
	int x0 = lua_tointeger(L, 1);
	int y0 = lua_tointeger(L, 2);
	int x1 = lua_tointeger(L, 3);
	int y1 = lua_tointeger(L, 4);
	int col = luaL_optinteger(L, 5, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_oval(x0, y0, x1, y1, true);

	return 0;
}

int oval(lua_State* L) {
	int x0 = lua_tointeger(L, 1);
	int y0 = lua_tointeger(L, 2);
	int x1 = lua_tointeger(L, 3);
	int y1 = lua_tointeger(L, 4);
	int col = luaL_optinteger(L, 5, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_oval(x0, y0, x1, y1, false);

	return 0;
}

int circfill(lua_State* L) {
	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	int r = luaL_optinteger(L, 3, 4);
	int col = luaL_optinteger(L, 4, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_oval(x - r, y - r, x + r, y + r, true);

	return 0;
}

int circ(lua_State* L) {
	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	int r = luaL_optinteger(L, 3, 4);
	int col = luaL_optinteger(L, 4, p8_memory[ADDR_DS_COLOR]);

	p8_memory[ADDR_DS_COLOR] = col;
	renderer->draw_oval(x - r, y - r, x + r, y + r, false);

	return 0;
}

int cls(lua_State* L) {
	int col = luaL_optinteger(L, 1, 0);

	renderer->clear_screen(col);
	p8_memory[ADDR_DS_CURSOR_X] = 0;
	p8_memory[ADDR_DS_CURSOR_Y] = 0;

	return 0;
}

int spr(lua_State* L) {
	int n = lua_tointeger(L, 1);
	int x = lua_tointeger(L, 2);
	int y = lua_tointeger(L, 3);
	int w = luaL_optinteger(L, 4, 1.0) * 8;
	int h = luaL_optinteger(L, 5, 1.0) * 8;
	int flip_x = lua_toboolean(L, 6);
	int flip_y = lua_toboolean(L, 7);

	renderer->draw_sprite(n, x, y, w, h, flip_x, flip_y);

	return 0;
}

int sspr(lua_State* L) {
	int sx = lua_tointeger(L, 1);
	int sy = lua_tointeger(L, 2);
	int sw = lua_tointeger(L, 3);
	int sh = lua_tointeger(L, 4);
	int dx = lua_tointeger(L, 5);
	int dy = lua_tointeger(L, 6);
	int dw = luaL_optinteger(L, 7, sw);
	int dh = luaL_optinteger(L, 8, sh);
	int flip_x = lua_toboolean(L, 9);
	int flip_y = lua_toboolean(L, 10);

	renderer->draw_from_spritesheet(sx, sy, sw, sh, dx, dy, dw, dh, flip_x, flip_y);

	return 0;
}

int map(lua_State* L) {
	int cellx = lua_tointeger(L, 1);
	int celly = lua_tointeger(L, 2);
	int sx = lua_tointeger(L, 3);
	int sy = lua_tointeger(L, 4);
	int cellw = luaL_optinteger(L, 5, 128);
	int cellh = luaL_optinteger(L, 6, 64);
	int layer = luaL_optinteger(L, 7, 0);

	renderer->draw_map(cellx, celly, sx, sy, cellw, cellh, layer);

	return 0;
}

int mget(lua_State* L) {
	int cellx = luaL_optinteger(L, 1, 0);
	int celly = luaL_optinteger(L, 2, 0);
	if (cellx < 0 || celly < 0 || cellx >= 128 || celly >= 64) {
		lua_pushinteger(L, 0);
		return 1;
	}

	int row_offset = ADDR_MAP + celly * 128;
	if (celly >= 32) {
		row_offset = ADDR_MAP_SHARED + (celly - 32) * 128;
	}
	int n = p8_memory[row_offset + cellx];

	lua_pushinteger(L, n);

	return 1;
}

int mset(lua_State* L) {
	int cellx = luaL_optinteger(L, 1, 0);
	int celly = luaL_optinteger(L, 2, 0);
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

	int x, y;
	bool scroll = false;

	if (lua_isinteger(L, 4)) {
		x = lua_tointeger(L, 2);
		y = lua_tointeger(L, 3);
		int col = luaL_checkinteger(L, 4);

		p8_memory[ADDR_DS_COLOR] = col;
		font->print(text, x, y, false);
	}
	if (lua_isnumber(L, 3)) {
		x = lua_tointeger(L, 2);
		y = lua_tointeger(L, 3);
	} else {
		if (lua_isinteger(L, 2)) {
			int col = luaL_checkinteger(L, 2);
			p8_memory[ADDR_DS_COLOR] = col;
		}
		x = p8_memory[ADDR_DS_CURSOR_X];
		y = p8_memory[ADDR_DS_CURSOR_Y];
		scroll = true;
	}
	int width = font->print(text, x, y, scroll);

	lua_pushinteger(L, width);

	return 1;
}

int color(lua_State* L) {
	int col = luaL_optinteger(L, 1, 6);
	p8_memory[ADDR_DS_COLOR] = col;
	return 0;
}

void set_draw_pal(int c0, int c1) {
	p8_memory[ADDR_DS_DRAW_PAL + c0] = (p8_memory[ADDR_DS_DRAW_PAL + c0] & 0xf0) | (c1 & 0x0f);
}
void set_screen_pal(int c0, int c1) {
	p8_memory[ADDR_DS_SCREEN_PAL + c0] = c1;
}
int pal(lua_State* L) {
	if (lua_gettop(L) == 0) {
		renderer->reset_draw_pal();
		renderer->reset_screen_pal();
		return 0;
	}

	if (lua_istable(L, 1)) {
		int p = luaL_optinteger(L, 2, 0);
		lua_pushvalue(L, 1);
		lua_pushnil(L); // push key
		while (lua_next(L, -2) != 0) {
			int c0 = lua_tointeger(L, -2) & 0x0FF;
			int c1 = lua_tointeger(L, -1);

			if (p == 0) {
				set_draw_pal(c0, c1);
			}
			else if (p == 1) {
				set_screen_pal(c0, c1);
			}

			/* removes 'value'; keeps 'key' for next iteration */
			lua_pop(L, 1);
		}
		lua_pop(L, 1); // pop key

		return 0;
	}

	int c0 = lua_tointeger(L, 1);
	if (lua_gettop(L) == 1) {
		// pal(0) => Resets
		// pal(x) => Noop
		if (c0 == 0) {
			renderer->reset_draw_pal();
		}
		return 0;
	}

	int c1 = lua_tointeger(L, 2);
	int p = luaL_optinteger(L, 3, 0);
	if (p > 1) {
		alert_todo("pal for secondary palette");

		return 0;
	}

	if (p == 0) {
		set_draw_pal(c0, c1);
	} else if (p == 1) {
		set_screen_pal(c0, c1);
	}

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

	int x = luaL_optinteger(L, 1, 0);
	int y = luaL_optinteger(L, 2, 0);

	memory_write_short(ADDR_DS_CAMERA_X, x);
	memory_write_short(ADDR_DS_CAMERA_Y, y);

	lua_pushinteger(L, original_x);
	lua_pushinteger(L, original_y);

	return 2;
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
		}
		else {
			p8_memory[ADDR_SPRITE_FLAGS + sprite] &= ~bitmask;
		}
	}
	else {
		unsigned char value = luaL_checkinteger(L, 2);
		p8_memory[ADDR_SPRITE_FLAGS + sprite] = value;
	}

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
	int x = luaL_optinteger(L, 1, 0);
	int y = luaL_optinteger(L, 2, 0);
	int w = luaL_optinteger(L, 3, P8_WIDTH);
	int h = luaL_optinteger(L, 4, P8_HEIGHT);
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

int flip(lua_State* L) {
	renderer->present();
	renderer->syncrhonize_30fps();

	return 0;
}

void load_draw_fns(lua_State* L)
{
	register_fn(L, "print", print);
	register_fn(L, "cls", cls);
	register_fn(L, "spr", spr);
	register_fn(L, "sspr", sspr);
	register_fn(L, "map", map);
	register_fn(L, "mget", mget);
	register_fn(L, "mset", mset);
	register_fn(L, "cursor", cursor);
	register_fn(L, "color", color);
	register_fn(L, "pget", pget);
	register_fn(L, "pset", pset);
	register_fn(L, "fillp", fillp);
	register_fn(L, "line", line);
	register_fn(L, "tline", tline);
	register_fn(L, "rect", rect);
	register_fn(L, "rectfill", rectfill);
	register_fn(L, "circ", circ);
	register_fn(L, "circfill", circfill);
	register_fn(L, "oval", oval);
	register_fn(L, "ovalfill", ovalfill);
	register_fn(L, "pal", pal);
	register_fn(L, "palt", palt);
	register_fn(L, "camera", camera);
	register_fn(L, "fget", fget);
	register_fn(L, "fset", fset);
	register_fn(L, "sget", sget);
	register_fn(L, "sset", sset);
	register_fn(L, "clip", clip);
	register_fn(L, "flip", flip);
}
