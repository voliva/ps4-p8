#include "memory.h"
#include "log.h"

char p8_memory[P8_TOTAL_MEMORY];

#define DEBUGLOG Memory_DEBUGLOG
Log DEBUGLOG = logger.log("memory");

void memory_load_cartridge(Cartridge& cartrige)
{
	// Reset pointers
	p8_memory[ADDR_SPRITE_SHEET_POINTER] = 0x00;
	p8_memory[ADDR_SCREEN_POINTER] = 0x60;
	p8_memory[ADDR_MAP_POINTER] = 0x20;

	memcpy(&p8_memory[ADDR_SPRITE_SHEET], &cartrige.sprite_map[0], 0x2000);
	memcpy(&p8_memory[ADDR_MAP], &cartrige.sprite_map[0x2000], 0x1000);
	memcpy(&p8_memory[ADDR_SPRITE_FLAGS], &cartrige.sprite_flags[0], 0x0100);
	memcpy(&p8_memory[ADDR_MUSIC], &cartrige.music[0], 0x0200);
	memcpy(&p8_memory[ADDR_SFX], &cartrige.sfx[0], 0x1100);

	// Reset the rest to zero
	memset(&p8_memory[0x4300], 0, P8_TOTAL_MEMORY - 0x4300);

	// Except the memory pointers :'D
	p8_memory[ADDR_SPRITE_SHEET_POINTER] = 0x00;
	p8_memory[ADDR_SCREEN_POINTER] = 0x60;
	p8_memory[ADDR_MAP_POINTER] = 0x20;
}

void memory_write_short(unsigned int addr, unsigned short value)
{
	p8_memory[addr] = value & 0x0FF;
	p8_memory[addr + 1] = value >> 8;
}

void memory_write_int(unsigned int addr, unsigned int value)
{
	memory_write_short(addr, value & 0x0FFFF);
	memory_write_short(addr+2, value >> 16);
}

void memory_write_long(unsigned int addr, unsigned long long value)
{
	memory_write_int(addr, value & 0x0FFFFFFFF);
	memory_write_int(addr + 4, value >> 32);
}

unsigned short memory_read_short(unsigned int addr)
{
	unsigned short res;
	
	res = p8_memory[addr];
	res |= (short)p8_memory[addr + 1] << 8;

	return res;
}

unsigned int memory_read_int(unsigned int addr)
{
	unsigned int res;

	res = memory_read_short(addr);
	res |= (int)memory_read_short(addr + 2) << 16;

	return res;
}

unsigned long long memory_read_long(unsigned int addr)
{
	unsigned long long res;

	res = memory_read_int(addr);
	res |= (long long)memory_read_int(addr + 4) << 32;

	return res;
}
