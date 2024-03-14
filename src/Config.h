#pragma once

#include <vulkan/vulkan.h>
#include <cassert>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#undef min
#undef max
#endif