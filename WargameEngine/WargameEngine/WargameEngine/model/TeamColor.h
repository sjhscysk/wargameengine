#pragma once
#include <string>
#include <string.h>

struct sTeamColor
{
	std::wstring suffix;
	unsigned char color[3];
};

inline bool operator< (sTeamColor const& one, sTeamColor const& two) { return one.suffix < two.suffix || memcmp(one.color, two.color, 3) < 0; }