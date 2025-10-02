#include "pause_menu.h"
#include "renderer.h"
#include "memory.h"
#include "font.h"
#include "running-cart.h"
#include "lua_state.h"
#include "machine_state.h"
#include "save_states.h"
#include "audio.h"
#include "file_paths.h"
#include "machine_state.h"

#define DEFAULT_LINES 6
#define OPTION_LINES 5

void load_settings();
void save_settings();

void PauseMenu::initialize()
{
	this->active_index = 0;
	this->active_screen = 0;
	this->pressed = false;

	load_settings();
}

void PauseMenu::manageEvent(KeyEvent& e)
{
	if (!e.down) {
		return;
	}

	unsigned char lines = this->active_screen == 0 ? DEFAULT_LINES : OPTION_LINES;
	switch (e.key) {
	case P8_Key::down:
		this->active_index = (this->active_index + 1) % lines;
		break;
	case P8_Key::up:
		this->active_index--;
		if (this->active_index == 0xFF) {
			this->active_index = lines-1;
		}
		break;
	case P8_Key::pause:
		if (this->active_screen != 0) {
			this->active_screen = 0;
			this->active_index = 0;
		}
		else {
			this->active_index = 0;
			runningCart->resume();
		}
		break;
	case P8_Key::right:
		if (this->active_screen == 1) {
			if (this->active_index == 1) {
				if (renderer->filter < FILTER_CRT) {
					renderer->filter++;
				}
				else {
					renderer->filter = FILTER_NONE;
				}
				renderer->present(true);
			}
			else if (this->active_index == 2) {
				if (audio_setting_sfx_volume < 10) {
					audio_setting_sfx_volume++;
				}
			}
			else if (this->active_index == 3) {
				if (audio_setting_music_volume < 10) {
					audio_setting_music_volume++;
				}
			}
			else if (this->active_index == 4) {
				machineState->controlInverted = !machineState->controlInverted;
			}
		}
		break;
	case P8_Key::left:
		if (this->active_screen == 1) {
			if (this->active_index == 1) {
				if (renderer->filter > 0) {
					renderer->filter--;
				}
				else {
					renderer->filter = FILTER_CRT;
				}
				renderer->present(true);
			}
			else if (this->active_index == 2) {
				if (audio_setting_sfx_volume > 0) {
					audio_setting_sfx_volume--;
				}
			}
			else if (this->active_index == 3) {
				if (audio_setting_music_volume > 0) {
					audio_setting_music_volume--;
				}
			}
			else if (this->active_index == 4) {
				machineState->controlInverted = !machineState->controlInverted;
			}
		}
		break;
	case P8_Key::circle:
	case P8_Key::cross:
		if (this->active_screen == 0) {
			switch (this->active_index) {
			case 0: // continue
				runningCart->resume();
				break;
			case 1:
				runningCart->restart();
				break;
			case 2:
				save_state(runningCart->getName());
				runningCart->resume();
				break;
			case 3:
				load_state(runningCart->getName());
				runningCart->resume();
				break;
			case 4:
				this->active_screen = 1;
				break;
			case 5:
				runningCart->stop();
				break;
			}
			this->active_index = 0;
		}
		else {
			if (this->active_index == 0) {
				this->active_index = 0;
				this->active_screen = 0;
				save_settings();
			}
			else if (this->active_index == 4) {
				machineState->controlInverted = !machineState->controlInverted;
			}
		}
		break;
	}
}


void render_filter_setting(int x, int y) {
	std::string title = "filter:";
	std::vector<std::string> options = { "none", "dot", "crt" };
	int option_lengths = 0;
	for (int i = 0; i < options.size(); i++) option_lengths += options[i].length();

	p8_memory[ADDR_DS_COLOR] = 7;
	font->print(title, x, y, false);
	x += 4 * title.length();

	for (int i = 0; i < options.size(); i++) {
		if (i == renderer->filter) {
			p8_memory[ADDR_DS_COLOR] = 12;
		}
		else {
			p8_memory[ADDR_DS_COLOR] = 7;
		}
		font->print(
			options[i],
			x,
			y,
			false
		);
		x += 4 * options[i].length() + 2;
	}

	p8_memory[ADDR_DS_COLOR] = 7;
}

void render_audio_setting(int x, int y, std::string title, int value) {
	std::string line = title + std::string(": ") + std::to_string(value);

	font->print(
		line,
		x,
		y,
		false
	);
}

void render_invert_control_setting(int x, int y) {
	std::string line = "invert buttons:" + std::string(machineState->controlInverted ? "yes" : "no");

	font->print(
		line,
		x,
		y,
		false
	);
}

void PauseMenu::draw()
{
	unsigned char original_color = p8_memory[ADDR_DS_COLOR];
	unsigned short original_camera_x = memory_read_short(ADDR_DS_CAMERA_X);
	unsigned short original_camera_y = memory_read_short(ADDR_DS_CAMERA_Y);
	memory_write_short(ADDR_DS_CAMERA_X, 0);
	memory_write_short(ADDR_DS_CAMERA_Y, 0);

	unsigned char lines = this->active_screen == 0 ? DEFAULT_LINES : OPTION_LINES;

	int height = 5 + lines * 8 - 3 + 5;
	int width = 2 * P8_WIDTH / 3;

	int left = (P8_WIDTH - width) / 2;
	int right = left + width;
	int top = (P8_HEIGHT - height) / 2;
	int bottom = top + height;

	p8_memory[ADDR_DS_COLOR] = 0;
	renderer->draw_rectangle(left, top, right, bottom, true);
	p8_memory[ADDR_DS_COLOR] = 7;
	renderer->draw_rectangle(left + 2, top + 2, right - 2, bottom - 2, false);

	for (int l = 0; l < lines; l++) {
		int text_top = top + 5 + l * 8;
		int text_x = left + 9;
		if (this->active_index == l) {
			// Arrow
			renderer->draw_line(left + 5, text_top, left + 5, text_top + 4);
			renderer->draw_line(left + 6, text_top+1, left + 6, text_top + 3);
			renderer->draw_point(left + 7, text_top+2);
			text_x += 1;
		}

		if (this->active_screen == 0) {
			switch (l) {
			case 0:
				font->print("continue", text_x, text_top, false);
				break;
			case 1:
				font->print("reset cart", text_x, text_top, false);
				break;
			case 2:
				font->print("save state", text_x, text_top, false);
				break;
			case 3:
				font->print("load state", text_x, text_top, false);
				break;
			case 4:
				font->print("options", text_x, text_top, false);
				break;
			case 5:
				font->print("quit", text_x, text_top, false);
				break;
			}
		}
		else {
			switch (l) {
			case 0:
				font->print("back", text_x, text_top, false);
				break;
			case 1:
				render_filter_setting(text_x, text_top);
				break;
			case 2:
				render_audio_setting(text_x, text_top, "sfx", audio_setting_sfx_volume);
				break;
			case 3:
				render_audio_setting(text_x, text_top, "music", audio_setting_music_volume);
				break;
			case 4:
				render_invert_control_setting(text_x, text_top);
				break;
			}
		}
	}

	renderer->present();
	p8_memory[ADDR_DS_COLOR] = original_color;
	memory_write_short(ADDR_DS_CAMERA_X, original_camera_x);
	memory_write_short(ADDR_DS_CAMERA_Y, original_camera_y);
}

void save_settings() {
	FILE* f = fopen(SETTINGS_FILE, "w");
	if (!f) {
		printf("Can't open settings file\n");
		return;
	}

	auto filter_value = renderer->filter == FILTER_NONE ? "none" : renderer->filter == FILTER_CRT ? "crt" : "dot";
	fprintf(f, "filter=%s\n", filter_value);
	fprintf(f, "sfx=%d\n", audio_setting_sfx_volume);
	fprintf(f, "music=%d\n", audio_setting_music_volume);
	fprintf(f, "inverted=%s\n", machineState->controlInverted ? "yes" : "no");

	fclose(f);
}

void load_settings() {
	FILE* f = fopen(SETTINGS_FILE, "r");
	if (f) {
		char line[1024];
		while (fgets(line, 1024, f)) {
			std::string s = line;
			int eq = s.find('=');
			if (eq == std::string::npos) continue;

			auto label = s.substr(0, eq);
			auto value = s.substr(eq + 1, s.length() - eq - 2);
			if (label == "filter") {
				if (value == "none") {
					renderer->filter = FILTER_NONE;
				}
				else if (value == "crt") {
					renderer->filter = FILTER_CRT;
				}
				else if (value == "dot") {
					renderer->filter = FILTER_DOT;
				}
			}
			else if (label == "sfx") {
				audio_setting_sfx_volume = std::stoi(value);
			}
			else if (label == "music") {
				audio_setting_music_volume = std::stoi(value);
			}
			else if (label == "inverted") {
				machineState->controlInverted = value == "yes";
			}
		}

		fclose(f);
	}
}