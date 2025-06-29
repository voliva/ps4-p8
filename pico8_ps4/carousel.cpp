#include "carousel.h"
#include "renderer.h"
#include "font.h"

Carousel::Carousel(int itemCount, int itemWidth, int itemHeight) {
	this->itemCount = itemCount;
	this->itemWidth = itemWidth;
	this->itemHeight = itemHeight;
	this->activeIndex = 0;
	this->renderingIndex = 0;
}
void Carousel::setItemcount(int itemCount) {
	this->itemCount = itemCount;
	if (this->activeIndex >= this->itemCount) {
		this->activeIndex = this->itemCount - 1;
	}
	if (this->activeIndex < 0) {
		this->activeIndex = 0;
	}
}
void Carousel::reset() {
	this->activeIndex = 0;
	this->renderingIndex = 0;
}

void Carousel::keyDown(Key key) {
	switch (key)
	{
	case Key::left:
		if (this->activeIndex == 0)
			break;
		this->activeIndex--;
		break;
	case Key::right:
		if (this->activeIndex == this->itemCount - 1)
			break;
		this->activeIndex++;
		break;
	}
}

std::vector<DrawItem> Carousel::draw(long long delta) {
	double target_diff = this->activeIndex - this->renderingIndex;
	double movement = target_diff * 0.1 * delta / 15;
	if (movement > 0)
	{
		if (movement < 0.01)
		{
			movement = 0.01;
		}
		if (this->renderingIndex + movement > this->activeIndex)
		{
			movement = this->activeIndex - this->renderingIndex;
		}
	}
	else if (movement < 0)
	{
		if (movement > -0.01)
		{
			movement = -0.01;
		}
		if (this->renderingIndex + movement < this->activeIndex)
		{
			movement = this->activeIndex - this->renderingIndex;
		}
	}
	this->renderingIndex += movement;

	std::vector<DrawItem> result = {};

	for (int i = 0; i < this->itemCount; i++)
	{
		double rendering_diff = fabs(this->renderingIndex - i);
		if (rendering_diff > 2.2)
		{
			continue;
		}

		int alpha = 255 - 255 * rendering_diff / 2.2;
		if (alpha < 0)
			alpha = 0;

		double scale = 1.2 - 0.2 * rendering_diff;
		if (scale < 1)
			scale = 1;

		int x_center = FRAME_WIDTH / 2 + (this->itemWidth + 100) * ((double)i - this->renderingIndex);
		double x = x_center - this->itemWidth * scale / 2;
		double y = 30 + SYS_CHAR_HEIGHT + 100 + (1 - scale) * this->itemHeight / 2;
		double w = (double)this->itemWidth * scale;
		double h = (double)this->itemHeight * scale;

		SDL_Rect dest{
			(int)x,
			(int)y,
			(int)w,
			(int)h };

		result.push_back(DrawItem{
			i,
			alpha,
			dest
		});
	}

	return result;
}

int Carousel::getActiveIndex() {
	return round(this->renderingIndex);
}
