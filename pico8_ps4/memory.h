#pragma once
#include "cartridge.h"

#define P8_TOTAL_MEMORY 0x10000
extern unsigned char p8_memory[P8_TOTAL_MEMORY];

// Pointers
#define ADDR_SPRITE_SHEET_POINTER 0x5F54
#define ADDR_SCREEN_POINTER 0x5F55
#define ADDR_MAP_POINTER 0x5F56

// Addresses
#define ADDR_SPRITE_SHEET (p8_memory[ADDR_SPRITE_SHEET_POINTER] << 8)
#define ADDR_MAP_SHARED 0x1000
#define ADDR_MAP (p8_memory[ADDR_MAP_POINTER] << 8)
#define ADDR_SPRITE_FLAGS 0x3000
#define ADDR_MUSIC 0x3100
#define ADDR_SFX 0x3200
#define ADDR_CUSTOM_FONT 0x5600
#define ADDR_PERSISTENT 0x5E00
#define ADDR_DRAW_STATE 0x5F00
#define ADDR_HARDWARE_STATE 0x5F40
#define ADDR_GPIO 0x5F80
#define ADDR_SCREEN (p8_memory[ADDR_SCREEN_POINTER] << 8)

/// Detailed pointers ///
// Draw state
#define ADDR_DS_DRAW_PAL 0x5F00
#define ADDR_DS_SCREEN_PAL 0x5F10
#define ADDR_DS_CLIP_RECT 0x5F20
#define ADDR_DS_CURSOR_HOME_X 0x5F24
#define ADDR_DS_COLOR 0x5F25
#define ADDR_DS_CURSOR_X 0x5F26
#define ADDR_DS_CURSOR_Y 0x5F27
#define ADDR_DS_CAMERA_X 0x5F28
#define ADDR_DS_CAMERA_Y 0x5F2A
#define ADDR_DS_SCREEN_TRANSFORM 0x5F2C
// ??
#define ADDR_DS_FILL_PAT 0x5F31
// ??
#define ADDR_DS_LINE_ENDPOINT_VALID 0x5F35
#define ADDR_DS_TLINE_MOD 0x5F38
#define ADDR_DS_TLINE_OFFSET 0x5F3A
#define ADDR_DS_LINE_ENDPOINT 0x5F3C

// Hardware state
#define ADDR_HW_AUDIO_SAMPLE_PITCH 0x5F40
#define ADDR_HW_AUDIO_REBERB 0x5F41
#define ADDR_HW_AUDIO_BITCRUSH 0x5F42
#define ADDR_HW_AUDIO_DAMPEN 0x5F43
#define ADDR_HW_RAND_STATE 0x5F44
#define ADDR_HW_BTN_STATES 0x5F4C
#define ADDR_HW_MAP_WIDTH 0x5F57
#define ADDR_HW_PRINT_ATTRS 0x5F58
#define ADDR_HW_BTNP_DELAY 0x5F5C
#define ADDR_HW_BTNP_INTERVAL 0x5F5D

void memory_load_cartridge(Cartridge& cartrige);

// Uses LE (least significant byte first) to write N bytes
void memory_write_short(unsigned int addr, unsigned short value);
void memory_write_int(unsigned int addr, unsigned int value);
void memory_write_long(unsigned int addr, unsigned long long value);
unsigned short memory_read_short(unsigned int addr);
unsigned int memory_read_int(unsigned int addr);
unsigned long long memory_read_long(unsigned int addr);