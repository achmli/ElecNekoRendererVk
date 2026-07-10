//
//  application.h
//
#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <filesystem>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "Math/Quat.h"
#include "Math/Vector.h"
#include "Physics/Body.h"
#include "Physics/Shapes.h"

#include "RHI/DeviceContext.h"
#include "RHI/ElecNekoShader.h"
#include "RHI/FrameBuffer.h"
#include "RHI/model.h"
#include "RHI/shader.h"
#include "RHI2/RHIBuffer.h"
#include "RHI2/RHIDevice.h"
#include "RHI2/RHISampler.h"

#include "Scene/Camera.h"
#include "Scene/CascadedShadow.h"
#include "Scene/Light.h"
#include "Scene/SkyBox.h"


#include "RenderOption.h"

#include "Renderer/Scene/RenderScene.h"

/*
====================================================
Application
====================================================
*/
class Application
{
public:
    Application() : m_isPaused(true), m_stepFrame(false) {}

    ~Application();

    void Initialize();

    void MainLoop();

private:
    std::vector<const char *> GetGLFWRequiredExtensions() const;

    void InitializeGLFW();

    bool InitializeImGui();

    bool InitializeVulkan();

    void Cleanup();

    void UpdateUniforms();

    void DrawFrame();

    void ResizeWindow(int windowWidth, int windowHeight);

    void ReloadScene();

    void MouseMoved(float x, float y);

    void LeftMouseMoved(float x, float y);

    void MouseScrolled(float z);

    void Keyboard(int key, int scancode, int action, int modifiers);

    void ProcessKeyboard(float deltaTime);

    static void OnWindowResized(GLFWwindow *window, int width, int height);

    static void OnMouseMoved(GLFWwindow *window, double x, double y);

    static void OnMouseWheelScrolled(GLFWwindow *window, double x, double y);

    static void OnKeyboard(GLFWwindow *window, int key, int scancode, int action, int modifiers);

    static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

private:
    // class Scene *m_scene;

    GLFWwindow *m_glfwWindow;

    DeviceContext m_deviceContext;
    std::unique_ptr<RHI::Device> m_rhiDevice;
    std::unique_ptr<RHI::Sampler> m_textureArraySamplerRHI;

    VkDescriptorPool m_imguiDescriptorPool;
    VkRenderPass m_imguiRenderPass;

    //
    //	Uniform Buffer
    //
    Buffer m_uniformBuffer;
    std::unique_ptr<RHI::Buffer> m_uniformBufferRHI;
    //
    //	Model
    //
    Model m_modelFullScreen;
    // std::vector<Model *> m_models; // models for the bodies

    ElecNeko::Camera m_camera;
    std::vector<ElecNeko::Light *> m_lights;

    std::vector<std::filesystem::path> m_sceneFiles;

    /*ElecNeko::Scene *m_scene;
    ElecNeko::Scene *deletingScene = nullptr;*/

    ElecNeko::RenderScene *m_renderScene = nullptr;
    // ElecNeko::RenderScene *deletingRenderScene = nullptr;

    ElecNeko::SkyBox m_skyBox;

    ElecNeko::OrthoCamera m_shadowCamera;
    // ElecNeko::Camera m_shadowCam;
    ElecNeko::CascadeShadow m_csm;

    ElecNeko::RenderOption renderOption;

    //
    //	Pipeline for copying the offscreen framebuffer to the swapchain
    //
    ElecNeko::ElecNekoShader m_copyShader;
    Descriptors m_copyDescriptors;
    Pipeline m_copyPipeline;

    // User input
    Vec2 m_mousePosition;

    bool m_isPaused;
    bool m_stepFrame;
    bool m_isMouseDown = false;
    bool m_isRightButtonDown = false;

    float m_cameraMoveSpeed = 5.0f;
    float m_mouseSensitivity = 0.1f;

    std::vector<RenderModel> m_renderModels;

    static const int WINDOW_WIDTH = 2560;
    static const int WINDOW_HEIGHT = 1440;

    static const bool m_enableLayers = true;
};

extern Application *g_application;
