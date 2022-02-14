#include "saves.h"
#include "memory.h"
#include "log.h"

#ifdef __PS4__
#define SAVE_FOLDER "/data/p8-saves"
#else
#define SAVE_FOLDER "../p8-saves"
#endif

#define PERSISTENT_SIZE 256

#define DEBUGLOG Saves_DEBUGLOG
Log DEBUGLOG = logger.log("Saves");

SaveManager::SaveManager() {
	this->dirty = false;
	this->name = "";
}

void SaveManager::initialize()
{
	this->name = "";
	this->dirty = false;
}

void SaveManager::open(std::string name)
{
	this->name = name;
	this->dirty = false;

	memset(&p8_memory[ADDR_PERSISTENT], 0, PERSISTENT_SIZE);
	std::string path = std::string(SAVE_FOLDER) + "/" + name + ".dat";
	FILE* f = fopen(path.c_str(), "r");
	if (f) {
		DEBUGLOG << "Loaded" << ENDL;
		fread(&p8_memory[ADDR_PERSISTENT], 1, PERSISTENT_SIZE, f);
		fclose(f);
	}
	else {
		DEBUGLOG << "Could not load, error: " << errno << ENDL;
	}
}

int SaveManager::read(int index)
{
	if (index >= 64 || index < 0) {
		return 0;
	}
	// DEBUGLOG << "read " << index << ": " << memory_read_int(ADDR_PERSISTENT + index * 4) << ENDL;
	return memory_read_int(ADDR_PERSISTENT + index * 4);
}

void SaveManager::write(int index, int value)
{
	if (index >= 64 || index < 0) {
		return;
	}
	memory_write_int(ADDR_PERSISTENT + index * 4, value);
	this->dirty = true;
}

void SaveManager::poke(unsigned short addr, unsigned char value)
{
	if (addr >= ADDR_PERSISTENT && addr < ADDR_PERSISTENT + PERSISTENT_SIZE) {
		this->dirty = true;
	}
}

void SaveManager::persist()
{
	if (this->dirty && this->name != "") {
		this->dirty = false;
		std::string path = std::string(SAVE_FOLDER) + "/" + name + ".dat";
		FILE* f = fopen(path.c_str(), "w");
		if (f) {
			fwrite(&p8_memory[ADDR_PERSISTENT], 1, PERSISTENT_SIZE, f);
			fclose(f);
		}
		else {
			alert_todo("Could not save, error: " + std::to_string(errno));
		}
	}
}
