#pragma once

#define PI 3.14159265358979323846f

#include <cmath>
#include <algorithm>
#include "Config.h"

namespace ElecNekoMath
{
	struct Math
	{
		static inline float Degrees(float radians) { return radians * (180.f / PI); }
		static inline float Radians(float degrees) { return degrees * PI / 180.f; }
		static inline float Clamp(float x, float lower, float upper) { return std::max(lower, std::min(upper, x)); }
	};
}