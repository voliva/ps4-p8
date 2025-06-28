#pragma once
#include "splore_loader.h"
#include "events.h"

enum class Mode {
	Featured,
	New
};

class Splore
{
public:
	void initialize(Mode m);
	void key_down(Key k);
	void render();

private:
	std::vector<SploreCartridge> cartridges;
	SDL_Texture* texture = NULL;
	int focus;
};
extern bool invalidateLocalCartridges;