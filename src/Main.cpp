#include "build.hpp"

#include <chrono>
#include <cmath>
#include <cstring>

#include <vulkan/vulkan.hpp>

#include "Application.hpp"

#if ENS_VULKAN && ENS_IMGUI
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo.h"
#endif

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>


/*bool done;

bool InitializeWindow(VkExtent2D size, bool bFullScreen = false, bool bResizeable = true, bool bLimitFrameRate = true);
void TerminateWindow();

void MainLoop(LoopData& arg)
{
	SDL_Event event;

	uint64_t startTime=SDL_GetPerformanceCounter();
	uint64_t lastTime=startTime;
	while (SDL_PollEvent(&event))
	{
#if ENS_IMGUI
		ImGui_ImplSDL2_ProcessEvent(&event);
#endif
		if (event.type == SDL_QUIT)
		{
			done = true;
		}

	}
}*/

int main(int argc, char** argv)
{
	/*if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		std::printf("Error to initial SDL: %s\n", SDL_GetError());
		return -1;
	}

	LoopData loopData;

	SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN);
	loopData.mWindow = SDL_CreateWindow("Electirc Neko Surface", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, windowFlags);
#if ENS_VULKAN
	const VkApplicationInfo app = {
	.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
	.pNext = nullptr,
	.pApplicationName = "ElecNekoSurface",
	.applicationVersion = 0,
	.pEngineName = "ElecNekoSurface",
	.engineVersion = 0,
	.apiVersion = VK_MAKE_VERSION(1, 0, 0),
	};
	
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
	createInfo.ppEnabledLayerNames = validationLayers.data();
#endif
	if (!loopData.mWindow)
	{
		std::printf("Error: Failed to create window!\n");
		return -1;
	}

	done = false;

	loopData.device->

	SDL_Surface* SDLsurface = SDL_GetWindowSurface(loopData.mWindow);

	SDL_FillRect(SDLsurface, NULL, SDL_MapRGB(SDLsurface->format, 0, 0, 0));

	while (!done)
	{
		MainLoop(loopData);
	}

	SDL_DestroyWindow(loopData.mWindow);

	SDL_Quit();

	return 0;*/

	Application *application=new Application();

	application->Initialize();
	while(!application->IsDone())
		application->MainLoop();

	delete application;
	return 0;
}