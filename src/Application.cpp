#include <chrono>
#include <thread>

#include "Application.hpp"

#if ENS_VULKAN && ENS_IMGUI
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo.h"
#endif

#include <cassert>
#include <windows.h>
#include <ctime>

void Application::Initialize()
{
    InitSDL();
    InitVulkan();
}

Application::~Application()
{
    Cleanup();
}

void Application::InitSDL()
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER)!=0)
    {
        std::printf("Error to initial SDL: %s\n", SDL_GetError());
        return;
    }

    SDL_WindowFlags sdlWindowFlags=(SDL_WindowFlags)(SDL_WINDOW_RESIZABLE|SDL_WINDOW_VULKAN|SDL_WINDOW_ALLOW_HIGHDPI);

    // Todo: I dont think its right. but just let it be.
    sdlWindow=SDL_CreateWindow("Electric Neko Surface",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WINDOW_WIDTH,WINDOW_HEIGHT,sdlWindowFlags);

    if(!sdlWindow)
    {
        std::printf("Error: Failed to create sdl window!\n");
        return;
    }
    // Todo: still not clear if its right. Let it be.
    //std::unique_ptr<SDL_Surface> sdlSurface = std::make_unique<SDL_Surface>();
    //sdlSurface.reset(SDL_GetWindowSurface(sdlWindow));
    //SDL_Surface* sdlSurface=SDL_GetWindowSurface(sdlWindow);
    SDL_Vulkan_CreateSurface(sdlWindow,deviceContext.vkInstance,&deviceContext.vkSurface);
}


std::vector<const char*> Application::GetSDLRequiredExtensions() const
{
    std::vector<const char*> extensions;
    // is it correct? idk, so just test it.
    const char** sdlExtensions = nullptr;
    uint32_t sdlExtensionsCount=0;
    SDL_Vulkan_GetInstanceExtensions(sdlWindow,&sdlExtensionsCount,sdlExtensions);

    for(int i=0;i<sdlExtensionsCount;i++)
    {
        extensions.emplace_back(sdlExtensions[i]);
    }

    if(bEnableLayers==true)
    {
        extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    return extensions;
}


bool Application::InitVulkan()
{
    {
        std::vector<const char*> extensions=GetSDLRequiredExtensions();
        if(!deviceContext.CreateInstance(bEnableLayers,extensions))
        {
            std::printf("Error: Failed to create vulkan instance!\n");
            assert(0);
            return false;
        }
    }

    {
        if(SDL_Vulkan_CreateSurface(sdlWindow,deviceContext.vkInstance,&deviceContext.vkSurface)!=SDL_TRUE)
        {
            std::printf("Error: Failed to create vulkan surface!\n");
            assert(0);
            return false;
        }
    }

    int windowWidth;
    int windowHeight;
    SDL_GetWindowSize(sdlWindow,&windowWidth,&windowHeight);

    if(!deviceContext.CreateDevice())
    {
        std::printf("Error: Failed to create Device!\n");
        assert(0);
        return false;
    }

    return true;
}

void Application::Cleanup()
{
    deviceContext.Cleanup();

    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
}

void Application::MainLoop()
{
    SDL_Event sdlEvent;

    while(SDL_PollEvent(&sdlEvent))
    {
        if(sdlEvent.type==SDL_QUIT)
        {
            bDone=true;
        }
    }
}
