// Extremely Fast Line Algorithm Var E (Addition Fixed Point PreCalc ModeX)
// Copyright 2001-2, By Po-Han Lin

// Freely useable in non-commercial applications as long as credits 
// to Po-Han Lin and link to http://www.edepot.com is provided in 
// source code and can been seen in compiled executable.  
// Commercial applications please inquire about licensing the algorithms.
//
// Lastest version at http://www.edepot.com/phl.html
// Note: This version is for small displays like on cell phones.
// with 256x256 max resolution.  For displays up to 65536x65536
// please visit http://www.edepot.com/linee.html

#include <stdlib.h>
#include "efla.e.h"

// THE EXTREMELY FAST LINE ALGORITHM Variation E (Addition Fixed Point PreCalc Small Display)
// Small Display (256x256) resolution.
std::vector<Renderer_Point> efla_small_line(int x, int y, int x2, int y2) {
	std::vector<Renderer_Point> result;
	bool yLonger = false;
	int shortLen = y2 - y;
	int longLen = x2 - x;
	if (abs(shortLen) > abs(longLen)) {
		int swap = shortLen;
		shortLen = longLen;
		longLen = swap;
		yLonger = true;
	}
	int decInc;
	if (longLen == 0) decInc = 0;
	else decInc = (shortLen << 8) / longLen;

	if (yLonger) {
		if (longLen > 0) {
			longLen += y;
			for (int j = 0x80 + (x << 8); y <= longLen; ++y) {
				result.push_back(Renderer_Point{
					j >> 8, y
				});
				j += decInc;
			}
			return result;
		}
		longLen += y;
		for (int j = 0x80 + (x << 8); y >= longLen; --y) {
			result.push_back(Renderer_Point{
				j >> 8, y
			});
			j -= decInc;
		}
		return result;
	}

	if (longLen > 0) {
		longLen += x;
		for (int j = 0x80 + (y << 8); x <= longLen; ++x) {
			result.push_back(Renderer_Point{
				x, j >> 8
			});
			j += decInc;
		}
		return result;
	}
	longLen += x;
	for (int j = 0x80 + (y << 8); x >= longLen; --x) {
		result.push_back(Renderer_Point{
			x, j >> 8
		});
		j -= decInc;
	}
	return result;
}
