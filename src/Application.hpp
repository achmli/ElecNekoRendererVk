#pragma once

#include "build.hpp"

#include <common.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "DeviceContext.hpp"
#include <memory>
#include <cstdio>

class Application
{
public:
	Application()=default;
	~Application();

	void Initialize();
	void MainLoop();

	bool IsDone();

private:
	std::vector<const char*> GetSDLRequiredExtensions() const;

	void InitSDL();
	bool InitVulkan();

	void Cleanup();
	void UpdateUniforms();
	void DrawFrame();
	void ResizeWindow(const int windowWidth,const int windowHeight);

	void ComputeFps(float &total,int &fps,uint32_t maxFps);

private:
	SDL_Window* sdlWindow=nullptr;

	DeviceContext deviceContext;

	static const int WINDOW_WIDTH=1600;
	static const int WINDOW_HEIGHT=900;

	static const bool bEnableLayers=true;
	bool bDone=false;
};

inline void Application::ComputeFps(float &total,int &fps,uint32_t maxFps)
{
	static uint64_t start=SDL_GetPerformanceCounter();
	static uint64_t last1=SDL_GetPerformanceCounter();
	static uint64_t last2=SDL_GetPerformanceCounter();
	// Get Current Time
	uint64_t currentTime=SDL_GetPerformanceCounter();

	// Get Delta Time
	float deltaTime1=static_cast<float>(currentTime-last1)/static_cast<float>(SDL_GetPerformanceFrequency());
	float deltaTime2=static_cast<float>(currentTime-last2)/static_cast<float>(SDL_GetPerformanceFrequency());
	// Update last frame time
	last1=currentTime;

	// compute total time
	total=static_cast<float>(currentTime-start)/static_cast<float>(SDL_GetPerformanceFrequency());
	// limit fps
	if(maxFps>0)
	{
		if(deltaTime1<1.f/static_cast<float>(maxFps))
		{
			deltaTime1 = 1.f/static_cast<float>(maxFps);
			SDL_Delay(static_cast<uint32_t>((1.f/static_cast<float>(maxFps)-deltaTime1)*1000));
		}
	}
	// update fps gap
	if(deltaTime2>.1f)
	{
		fps=static_cast<int>(1.f/deltaTime1);
		last2=currentTime;
	}
}

inline bool Application::IsDone()
{
	return bDone;
}
