#pragma once
#include "splore_loader.h"
#include "events.h"
#include "carousel.h"

enum class Mode {
	Featured,
	New
};

class Splore
{
public:
	void initialize(Mode m);
	void key_down(Key k);
	void render(long long delta);

private:
	std::vector<SploreCartridge> cartridges;
	SDL_Texture* texture = NULL;
	Carousel* carousel;
};
extern bool invalidateLocalCartridges;