#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "DeviceContext.hpp"
#include <memory>
#include <cstdio>

struct LoopData
{
	SDL_Window* mWindow = nullptr;
	std::unique_ptr<DeviceContext> device;
};

