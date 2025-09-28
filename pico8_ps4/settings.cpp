#include "settings.h"
#include "font.h"
#include "renderer.h"
#include "audio.h"
#include <string>
#include <vector>
#include "file_paths.h"

void Settings::initialize() {
	this->focusIdx = 0;

	FILE* f = fopen(SETTINGS_FILE, "r");
	if (f) {
		char line[1024];
		while (fgets(line, 1024, f)) {
			std::string s = line;
			int eq = s.find('=');
			if (eq == std::string::npos) continue;

			auto label = s.substr(0, eq);
			auto value = s.substr(eq + 1, s.length()-eq-2);
			if (label == "filter") {
				printf("value: '%s'", value.c_str());
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
		}

		fclose(f);
	}
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

	fclose(f);
}

void Settings::key_down(Key k)
{
	switch (k) {
	case Key::up:
		if (this->focusIdx > 0)
			this->focusIdx--;
		break;
	case Key::down:
		if (this->focusIdx < 2)
			this->focusIdx++;
		break;
	case Key::left:
		if (this->focusIdx == 0) {
			if (renderer->filter > 0) {
				renderer->filter--;
				save_settings();
			}
		}
		else if (this->focusIdx == 1) {
			if (audio_setting_sfx_volume > 0) {
				audio_setting_sfx_volume--;
				save_settings();
			}
		}
		else {
			if (audio_setting_music_volume > 0) {
				audio_setting_music_volume--;
				save_settings();
			}
		}
		break;
	case Key::right:
		if (this->focusIdx == 0) {
			if (renderer->filter < FILTER_DOT) {
				renderer->filter++;
				save_settings();
			}
		}
		else if (this->focusIdx == 1) {
			if (audio_setting_sfx_volume < 10) {
				audio_setting_sfx_volume++;
				save_settings();
			}
		}
		else {
			if (audio_setting_music_volume < 10) {
				audio_setting_music_volume++;
				save_settings();
			}
		}
		break;
	}
}

void render_filter_setting(int y, bool active) {
	std::string title = "filter:";
	std::vector<std::string> options = { "none", "crt", "dot" };
	int option_lengths = 0;
	for (int i = 0; i < options.size(); i++) option_lengths += options[i].length();

	int filter_line_width = SYS_CHAR_WIDTH * (title.length() + option_lengths + options.size());
	int x_offset = (FRAME_WIDTH - filter_line_width) / 2;

	int white = active ? 0xFF : 0xA0;
	SDL_SetRenderDrawColor(renderer->renderer, white, white, white, 0xFF);

	font->sys_print(
		title,
		x_offset,
		y
	);
	x_offset += SYS_CHAR_WIDTH * (1 + title.length());

	for (int i = 0; i < options.size(); i++) {
		if (i == renderer->filter) {
			SDL_SetRenderDrawColor(renderer->renderer, 0, white * 0.5, white, 0xFF);
		}
		else {
			SDL_SetRenderDrawColor(renderer->renderer, white * 0.8, white * 0.8, white * 0.8, 0xFF);
		}
		font->sys_print(
			options[i],
			x_offset,
			y
		);
		x_offset += SYS_CHAR_WIDTH * (1 + options[i].length());
	}
	SDL_SetRenderDrawColor(renderer->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
}

void render_audio_sfx_setting(int y, bool active, std::string title, int value) {
	std::string line = title + std::string(": ") + std::to_string(value);

	int sfx_line_width = SYS_CHAR_WIDTH * (line.length() + 3);
	int x_offset = (FRAME_WIDTH - sfx_line_width) / 2;

	int white = active ? 0xFF : 0xA0;
	SDL_SetRenderDrawColor(renderer->renderer, white, white, white, 0xFF);
	font->sys_print(
		line,
		x_offset,
		y
	);
}

void Settings::render(long long delta)
{
	int offsest = 180;
	render_filter_setting(offsest, this->focusIdx == 0);
	render_audio_sfx_setting(offsest + SYS_CHAR_HEIGHT * 1.5, this->focusIdx == 1, "sfx", audio_setting_sfx_volume);
	render_audio_sfx_setting(offsest + 2 * SYS_CHAR_HEIGHT * 1.5, this->focusIdx == 2, "music", audio_setting_music_volume);
}
