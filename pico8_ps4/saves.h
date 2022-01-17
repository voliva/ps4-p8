#pragma once

#include <string>

class SaveManager
{
public:
	SaveManager();
	void initialize();
	void open(std::string name);
	int read(int index);
	void write(int index, int value);
	void poke(unsigned short addr, unsigned char value);
	void persist();

private:
	std::string name;
	bool dirty;
};
extern SaveManager* saveManager;
