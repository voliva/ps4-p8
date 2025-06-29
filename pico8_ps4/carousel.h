#pragma once


#include "events.h"
#include <vector>
#include <functional>

typedef struct {
	int idx;
	int alpha;
	SDL_Rect destRect;
} DrawItem;

class Carousel
{
public:
	Carousel(int itemCount, int itemWidth, int itemHeight);
	void keyDown(Key key);
	std::vector<DrawItem> draw(long long delta);

	void setItemcount(int itemCount);
	void reset();
	int getActiveIndex();

private:
	int itemCount;
	int itemWidth;
	int itemHeight;

	int activeIndex;
	double renderingIndex;
};

