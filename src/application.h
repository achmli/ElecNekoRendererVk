//
//  application.h
//
#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "Math/Quat.h"
#include "Math/Vector.h"
#include "Physics/Body.h"
#include "Physics/Shapes.h"

#include "RHI/DeviceContext.h"
#include "RHI/FrameBuffer.h"
#include "RHI/model.h"
#include "RHI/shader.h"

#include "Loader/Mesh.h"

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

    void MouseMoved(float x, float y);

    void MouseScrolled(float z);

    void Keyboard(int key, int scancode, int action, int modifiers);

    void ProcessKeyboard(float deltaTime);

    static void OnWindowResized(GLFWwindow *window, int width, int height);

    static void OnMouseMoved(GLFWwindow *window, double x, double y);

    static void OnMouseWheelScrolled(GLFWwindow *window, double x, double y);

    static void OnKeyboard(GLFWwindow *window, int key, int scancode, int action, int modifiers);

    static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

private:
    class Scene *m_scene;

    GLFWwindow *m_glfwWindow;

    DeviceContext m_deviceContext;

    VkDescriptorPool m_imguiDescriptorPool;
    VkRenderPass m_imguiRenderPass;

    //
    //	Uniform Buffer
    //
    Buffer m_uniformBuffer;

    //
    //	Model
    //
    Model m_modelFullScreen;
    std::vector<Model *> m_models; // models for the bodies

    std::vector<ElecNeko::Mesh *> m_meshes;

    //
    //	Pipeline for copying the offscreen framebuffer to the swapchain
    //
    Shader m_copyShader;
    Descriptors m_copyDescriptors;
    Pipeline m_copyPipeline;

    // User input
    Vec2 m_mousePosition;
    Vec3 m_cameraFocusPoint;
    float m_cameraPositionTheta;
    float m_cameraPositionPhi;
    float m_cameraRadius;
    bool m_isPaused;
    bool m_stepFrame;
    bool m_isMouseDown = false;

    Vec3 m_cameraPosition = Vec3(-.7f, -7, 3);
    float m_cameraYaw = -90.f;
    float m_cameraPitch = 0.f;
    Vec3 m_cameraFront = Vec3(0, 1, 0);
    Vec3 m_cameraUp = Vec3(0, 0, 1);
    Vec3 m_cameraRight = Vec3(1, 0, 0);
    float m_cameraMoveSpeed = 5.0f;
    float m_mouseSensitivity = 0.1f;

    std::vector<RenderModel> m_renderModels;

    static const int WINDOW_WIDTH = 2560;
    static const int WINDOW_HEIGHT = 1440;

    static const bool m_enableLayers = true;
};

extern Application *g_application;
