#pragma once
#include "splore_loader.h"
#include "events.h"

class Splore
{
public:
	void initialize();
	void key_down(Key k);
	void render();

private:
	std::vector<SploreCartridge> cartridges;
	SDL_Texture* texture = NULL;
	int focus;
};
