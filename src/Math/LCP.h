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