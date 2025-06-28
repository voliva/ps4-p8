#pragma once

#ifdef __PS4__
#define SAVE_FOLDER "/data/p8-saves"
#elif __SWITCH__
#define SAVE_FOLDER "/switch/switch-p8/saves"
#else
#define SAVE_FOLDER "../p8-saves"
#endif

#ifdef __PS4__
#define SAVE_STATES_FOLDER "/data/p8-savestates"
#elif __SWITCH__
#define SAVE_STATES_FOLDER "/switch/switch-p8/savestates"
#else
#define SAVE_STATES_FOLDER "../p8-savestates"
#endif

#ifdef __PS4__
#define BUNDLED_FOLDER "/app0/assets/misc"
#define CARTRIDGE_FOLDER "/data/p8-cartridges"
#elif __SWITCH__
#define BUNDLED_FOLDER "romfs:/misc"
#define CARTRIDGE_FOLDER "/switch/switch-p8/cartridges"
#else
#define BUNDLED_FOLDER "../assets/misc"
#define CARTRIDGE_FOLDER "../p8-cartridges"
#endif

void prepareFilePaths();

#pragma once
