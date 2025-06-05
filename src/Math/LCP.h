//
//	LCP.h
//
#pragma once
#include <algorithm>
#include "Matrix.h"
#include "Vector.h"

#define PI 3.14159265358979323846f
/*
====================================================
LCP_GaussSeidel
====================================================
*/
VecN LCP_GaussSeidel(const MatN &A, const VecN &b);

namespace ElecNeko
{
    static inline float Degrees(float radians) { return radians * (180.f / PI); };
    static inline float Radians(float degrees) { return degrees * (PI / 180.f); };
}
