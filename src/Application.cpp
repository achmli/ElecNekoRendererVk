//
//  application.cpp
//
#include <chrono>
#include <thread>

#include "RHI/DeviceContext.h"
#include "RHI/Samplers.h"
#include "RHI/model.h"
#include "RHI/shader.h"

#include <assert.h>
#include "Fileio.h"
#include "application.h"

#include "RHI/OffscreenRenderer.h"

#include "Scene.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

Application *g_application = NULL;

#include <time.h>
#include <windows.h>

static bool gIsInitialized(false);
static unsigned __int64 gTicksPerSecond;
static unsigned __int64 gStartTicks;

/*
====================================
GetTimeSeconds
====================================
*/
int GetTimeMicroseconds()
{
    if (false == gIsInitialized)
    {
        gIsInitialized = true;

        // Get the high frequency counter's resolution
        QueryPerformanceFrequency((LARGE_INTEGER *) &gTicksPerSecond);

        // Get the current time
        QueryPerformanceCounter((LARGE_INTEGER *) &gStartTicks);

        return 0;
    }

    unsigned __int64 tick;
    QueryPerformanceCounter((LARGE_INTEGER *) &tick);

    const double ticks_per_micro = (double) (gTicksPerSecond / 1000000);

    const unsigned __int64 timeMicro = (unsigned __int64) ((double) (tick - gStartTicks) / ticks_per_micro);
    return (int) timeMicro;
}

void CheckVkResult(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

/*
========================================================================================================

Application

========================================================================================================
*/

/*
====================================================
Application::Initialize
====================================================
*/
void Application::Initialize()
{
    FillDiamond();

    InitializeGLFW();
    InitializeVulkan();

    InitializeImGui();

    m_scene = new Scene;
    m_scene->Initialize();
    m_scene->Reset();

    /*m_models.reserve(m_scene->m_bodies.size());
    for (int i = 0; i < m_scene->m_bodies.size(); i++)
    {
        Model *model = new Model();
        model->BuildFromShape(m_scene->m_bodies[i].m_shape);
        model->MakeVBO(&m_deviceContext);

        m_models.push_back(model);
    }*/
    {
        /*m_meshes.emplace_back();
        if (!m_meshes[0].LoadFromFile(&m_deviceContext, "Tree"))
        {
            printf("Failed to Load Mesh!\n");
            assert(0);
        }

        m_meshes[0].MakeUBO(&m_deviceContext);

        for (auto &meshPart: m_meshes[0].m_meshParts)
        {
            meshPart.MakeVBO(&m_deviceContext);
        }*/
        ElecNeko::Mesh *mesh = new ElecNeko::Mesh();
        mesh->LoadFromFile(&m_deviceContext, "lost_empire");
        mesh->MakeUBO(&m_deviceContext);
        for (auto& meshPart : mesh->m_meshParts)
        {
            meshPart.MakeVBO(&m_deviceContext);
        }
        m_meshes.emplace_back(mesh);
    }

    m_mousePosition = Vec2(0, 0);
    m_cameraPositionTheta = acosf(-1.0f) / 2.0f;
    m_cameraPositionPhi = 0;
    m_cameraRadius = 15.0f;
    m_cameraFocusPoint = Vec3(0, 0, 3);

    m_isPaused = true;
    m_stepFrame = false;
}

/*
====================================================
Application::~Application
====================================================
*/
Application::~Application() { Cleanup(); }

/*
====================================================
Application::InitializeGLFW
====================================================
*/
void Application::InitializeGLFW()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_glfwWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "ElecNeko Surface", nullptr, nullptr);

    glfwSetWindowUserPointer(m_glfwWindow, this);
    glfwSetWindowSizeCallback(m_glfwWindow, Application::OnWindowResized);

    glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetInputMode(m_glfwWindow, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetCursorPosCallback(m_glfwWindow, Application::OnMouseMoved);
    glfwSetScrollCallback(m_glfwWindow, Application::OnMouseWheelScrolled);
    glfwSetKeyCallback(m_glfwWindow, Application::OnKeyboard);
    glfwSetMouseButtonCallback(m_glfwWindow, Application::MouseButtonCallback);
}

/*
====================================================
Application::GetGLFWRequiredExtensions
====================================================
*/
std::vector<const char *> Application::GetGLFWRequiredExtensions() const
{
    std::vector<const char *> extensions;

    const char **glfwExtensions;
    uint32_t glfwExtensionCount = 0;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (uint32_t i = 0; i < glfwExtensionCount; i++)
    {
        extensions.push_back(glfwExtensions[i]);
    }

    if (m_enableLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    return extensions;
}

/*
====================================================
Application::InitializeVulkan
====================================================
*/
bool Application::InitializeVulkan()
{
    //
    //	Vulkan Instance
    //
    {
        std::vector<const char *> extensions = GetGLFWRequiredExtensions();
        if (!m_deviceContext.CreateInstance(m_enableLayers, extensions))
        {
            printf("ERROR: Failed to create vulkan instance\n");
            assert(0);
            return false;
        }
    }

    //
    //	Vulkan Surface for GLFW Window
    //
    if (VK_SUCCESS !=
        glfwCreateWindowSurface(m_deviceContext.m_vkInstance, m_glfwWindow, nullptr, &m_deviceContext.m_vkSurface))
    {
        printf("ERROR: Failed to create window sruface\n");
        assert(0);
        return false;
    }

    int windowWidth;
    int windowHeight;
    glfwGetWindowSize(m_glfwWindow, &windowWidth, &windowHeight);

    //
    //	Vulkan Device
    //
    if (!m_deviceContext.CreateDevice())
    {
        printf("ERROR: Failed to create device\n");
        assert(0);
        return false;
    }

    //
    //	Create SwapChain
    //
    if (!m_deviceContext.CreateSwapChain(windowWidth, windowHeight))
    {
        printf("ERROR: Failed to create swapchain\n");
        assert(0);
        return false;
    }

    //
    //	Initialize texture samplers
    //
    Samplers::InitializeSamplers(&m_deviceContext);

    //
    //	Command Buffers
    //
    if (!m_deviceContext.CreateCommandBuffers())
    {
        printf("ERROR: Failed to create command buffers\n");
        assert(0);
        return false;
    }

    //
    //	Uniform Buffer
    //
    m_uniformBuffer.Allocate(&m_deviceContext, NULL, sizeof(float) * 16 * 4 * 128, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    //
    //	Offscreen rendering
    //
    InitOffscreen(&m_deviceContext, m_deviceContext.m_swapChain.m_windowWidth,
                  m_deviceContext.m_swapChain.m_windowHeight);

    //
    //	Full screen texture rendering
    //
    {
        bool result;
        FillFullScreenQuad(m_modelFullScreen);
        for (int i = 0; i < m_modelFullScreen.m_vertices.size(); i++)
        {
            m_modelFullScreen.m_vertices[i].xyz[1] *= -1.0f;
        }
        m_modelFullScreen.MakeVBO(&m_deviceContext);

        result = m_copyShader.Load(&m_deviceContext, "DebugImage2D");
        if (!result)
        {
            printf("ERROR: Failed to load copy shader\n");
            assert(0);
            return false;
        }

        Descriptors::CreateParms_t descriptorParms;
        memset(&descriptorParms, 0, sizeof(descriptorParms));
        descriptorParms.numUniformsFragment = 1;
        descriptorParms.numImageSamplers = 1;
        result = m_copyDescriptors.Create(&m_deviceContext, descriptorParms);
        if (!result)
        {
            printf("ERROR: Failed to create copy descriptors\n");
            assert(0);
            return false;
        }

        Pipeline::CreateParms_t pipelineParms;
        memset(&pipelineParms, 0, sizeof(pipelineParms));
        pipelineParms.renderPass = m_deviceContext.m_swapChain.m_vkRenderPass;
        pipelineParms.descriptors = &m_copyDescriptors;
        pipelineParms.shader = &m_copyShader;
        pipelineParms.width = m_deviceContext.m_swapChain.m_windowWidth;
        pipelineParms.height = m_deviceContext.m_swapChain.m_windowHeight;
        pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
        pipelineParms.depthTest = false;
        pipelineParms.depthWrite = false;
        result = m_copyPipeline.Create(&m_deviceContext, pipelineParms);
        if (!result)
        {
            printf("ERROR: Failed to create copy pipeline\n");
            assert(0);
            return false;
        }
    }

    return true;
}

bool Application::InitializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(m_glfwWindow, true);

    VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
    poolInfo.poolSizeCount = (uint32_t) IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes = poolSizes;
    vkCreateDescriptorPool(m_deviceContext.m_vkDevice, &poolInfo, nullptr, &m_imguiDescriptorPool);

    VkAttachmentDescription attachment = {};
    attachment.format = m_deviceContext.m_swapChain.m_vkColorImageFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachment = {};
    colorAttachment.attachment = 0;
    colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachment;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &attachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(m_deviceContext.m_vkDevice, &renderPassInfo, nullptr, &m_imguiRenderPass) != VK_SUCCESS)
    {
        printf("[ImGui]Error: Failed to Create ImGui Render Pass!\n");
        assert(0);
        return false;
    }

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = m_deviceContext.m_vkInstance;
    initInfo.PhysicalDevice = m_deviceContext.m_vkPhysicalDevice;
    initInfo.Device = m_deviceContext.m_vkDevice;
    initInfo.QueueFamily = m_deviceContext.m_graphicsFamilyIdx;
    initInfo.Queue = m_deviceContext.m_vkGraphicsQueue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = m_imguiDescriptorPool;
    initInfo.Allocator = nullptr;
    initInfo.MinImageCount =
            m_deviceContext.m_physicalDevices[m_deviceContext.m_deviceIndex].m_vkSurfaceCapabilities.minImageCount;
    initInfo.ImageCount =
            m_deviceContext.m_physicalDevices[m_deviceContext.m_deviceIndex].m_vkSurfaceCapabilities.minImageCount + 1;
    initInfo.CheckVkResultFn = nullptr;
    initInfo.RenderPass = m_imguiRenderPass;
    ImGui_ImplVulkan_Init(&initInfo);

    VkCommandBuffer cmdBuffer = m_deviceContext.BeginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture();
}


/*
====================================================
Application::Cleanup
====================================================
*/
void Application::Cleanup()
{
    CleanupOffscreen(&m_deviceContext);

    m_copyShader.Cleanup(&m_deviceContext);
    m_copyDescriptors.Cleanup(&m_deviceContext);
    m_copyPipeline.Cleanup(&m_deviceContext);
    m_modelFullScreen.Cleanup(m_deviceContext);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(m_deviceContext.m_vkDevice, m_imguiDescriptorPool, nullptr);
    vkDestroyRenderPass(m_deviceContext.m_vkDevice, m_imguiRenderPass, nullptr);

    // Delete the screen so that it can clean itself up
    delete m_scene;
    m_scene = NULL;

    // Delete models
    /*for (int i = 0; i < m_models.size(); i++)
    {
        m_models[i]->Cleanup(m_deviceContext);
        delete m_models[i];
    }
    m_models.clear();*/

    for (auto& mesh : m_meshes)
    {
        mesh->Cleanup(&m_deviceContext);
    }
    m_meshes.clear();

    // Delete Uniform Buffer Memory
    m_uniformBuffer.Cleanup(&m_deviceContext);

    // Delete Samplers
    Samplers::Cleanup(&m_deviceContext);

    // Delete Device Context
    m_deviceContext.Cleanup();

    // Delete GLFW
    glfwDestroyWindow(m_glfwWindow);
    glfwTerminate();
}

/*
====================================================
Application::OnWindowResized
====================================================
*/
void Application::OnWindowResized(GLFWwindow *window, int windowWidth, int windowHeight)
{
    if (0 == windowWidth || 0 == windowHeight)
    {
        return;
    }

    Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    application->ResizeWindow(windowWidth, windowHeight);
}

/*
====================================================
Application::ResizeWindow
====================================================
*/
void Application::ResizeWindow(int windowWidth, int windowHeight)
{
    m_deviceContext.ResizeWindow(windowWidth, windowHeight);

    //
    //	Full screen texture rendering
    //
    {
        bool result;
        m_copyPipeline.Cleanup(&m_deviceContext);

        Pipeline::CreateParms_t pipelineParms;
        memset(&pipelineParms, 0, sizeof(pipelineParms));
        pipelineParms.renderPass = m_deviceContext.m_swapChain.m_vkRenderPass;
        pipelineParms.descriptors = &m_copyDescriptors;
        pipelineParms.shader = &m_copyShader;
        pipelineParms.width = windowWidth;
        pipelineParms.height = windowHeight;
        pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
        pipelineParms.depthTest = false;
        pipelineParms.depthWrite = false;
        result = m_copyPipeline.Create(&m_deviceContext, pipelineParms);
        if (!result)
        {
            printf("Unable to build pipeline!\n");
            assert(0);
            return;
        }
    }
}

/*
====================================================
Application::OnMouseMoved
====================================================
*/
void Application::OnMouseMoved(GLFWwindow *window, double x, double y)
{
    Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    if (application->m_isMouseDown)
        application->MouseMoved((float) x, (float) y);
}

/*
====================================================
Application::MouseMoved
====================================================
*/
void Application::MouseMoved(float x, float y)
{
    Vec2 newPosition = Vec2(x, y);
    Vec2 ds = newPosition - m_mousePosition;
    m_mousePosition = newPosition;

    float sensitivity = 0.01f;
    m_cameraPositionTheta += ds.y * sensitivity;
    m_cameraPositionPhi += ds.x * sensitivity;

    if (m_cameraPositionTheta < 0.14f)
    {
        m_cameraPositionTheta = 0.14f;
    }
    if (m_cameraPositionTheta > 3.0f)
    {
        m_cameraPositionTheta = 3.0f;
    }
}

void Application::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            application->m_isMouseDown = true;
        } else if (action == GLFW_RELEASE)
        {
            application->m_isMouseDown = false;
        }
    }
}


/*
====================================================
Application::OnMouseWheelScrolled
====================================================
*/
void Application::OnMouseWheelScrolled(GLFWwindow *window, double x, double y)
{
    Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    application->MouseScrolled((float) y);
}

/*
====================================================
Application::MouseScrolled
====================================================
*/
void Application::MouseScrolled(float z)
{
    m_cameraRadius -= z;
    if (m_cameraRadius < 0.5f)
    {
        m_cameraRadius = 0.5f;
    }
}

/*
====================================================
Application::OnKeyboard
====================================================
*/
void Application::OnKeyboard(GLFWwindow *window, int key, int scancode, int action, int modifiers)
{
    Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    application->Keyboard(key, scancode, action, modifiers);
}

/*
====================================================
Application::Keyboard
====================================================
*/
void Application::Keyboard(int key, int scancode, int action, int modifiers)
{
    if (GLFW_KEY_R == key && GLFW_RELEASE == action)
    {
        m_scene->Reset();
    }
    if (GLFW_KEY_T == key && GLFW_RELEASE == action)
    {
        m_isPaused = !m_isPaused;
    }
    if (GLFW_KEY_Y == key && (GLFW_PRESS == action || GLFW_REPEAT == action))
    {
        m_stepFrame = m_isPaused && !m_stepFrame;
    }
    if (key == GLFW_KEY_ESCAPE && (GLFW_PRESS == action))
    {
        glfwSetWindowShouldClose(m_glfwWindow, GLFW_TRUE);
    }
}

/*
====================================================
Application::MainLoop
====================================================
*/
void Application::MainLoop()
{
    static int timeLastFrame = 0;
    static int numSamples = 0;
    static float avgTime = 0.0f;
    static float maxTime = 0.0f;

    while (!glfwWindowShouldClose(m_glfwWindow))
    {
        int time = GetTimeMicroseconds();
        float dt_us = (float) time - (float) timeLastFrame;
        // if (dt_us < 16000.0f)
        // {
        //     int x = 16000 - (int) dt_us;
        //     std::this_thread::sleep_for(std::chrono::microseconds(x));
        //     dt_us = 16000;
        //     time = GetTimeMicroseconds();
        // }
        timeLastFrame = time;
        printf("\ndt_ms: %.1f    ", dt_us * 0.001f);

        // Get User Input
        glfwPollEvents();

        // If the time is greater than 33ms (30fps)
        // then force the time difference to smaller
        // to prevent super large simulation steps.
        if (dt_us > 33000.0f)
        {
            dt_us = 33000.0f;
        }

        bool runPhysics = true;
        if (m_isPaused)
        {
            dt_us = 0.0f;
            runPhysics = false;
            if (m_stepFrame)
            {
                dt_us = 16667.0f;
                m_stepFrame = false;
                runPhysics = true;
            }
            numSamples = 0;
            maxTime = 0.0f;
        }
        float dt_sec = dt_us * 0.001f * 0.001f;

        // Run Update
        if (runPhysics)
        {
            int startTime = GetTimeMicroseconds();
            for (int i = 0; i < 2; i++)
            {
                m_scene->Update(dt_sec * 0.5f);
            }
            int endTime = GetTimeMicroseconds();

            dt_us = (float) endTime - (float) startTime;
            if (dt_us > maxTime)
            {
                maxTime = dt_us;
            }

            avgTime = (avgTime * float(numSamples) + dt_us) / float(numSamples + 1);
            numSamples++;

            printf("frame dt_ms: %.2f %.2f %.2f", avgTime * 0.001f, maxTime * 0.001f, dt_us * 0.001f);
        }

        // Draw the Scene
        DrawFrame();
    }
}

/*
====================================================
Application::UpdateUniforms
====================================================
*/
void Application::UpdateUniforms()
{
    m_renderModels.clear();

    uint32_t uboByteOffset = 0;
    uint32_t cameraByteOFfset = 0;
    uint32_t shadowByteOffset = 0;

    struct camera_t
    {
        Mat4 matView;
        Mat4 matProj;
        Mat4 pad0;
        Mat4 pad1;
    };
    camera_t camera;

    //
    //	Update the uniform buffers
    //
    {
        unsigned char *mappedData = (unsigned char *) m_uniformBuffer.MapBuffer(&m_deviceContext);

        //
        // Update the uniform buffer with the camera information
        //
        {
            Vec3 camPos = Vec3(10, 0, 5) * 1.25f;
            Vec3 camLookAt = Vec3(0, 0, 1);
            Vec3 camUp = Vec3(0, 0, 1);

            camPos.x = cosf(m_cameraPositionPhi) * sinf(m_cameraPositionTheta);
            camPos.y = sinf(m_cameraPositionPhi) * sinf(m_cameraPositionTheta);
            camPos.z = cosf(m_cameraPositionTheta);
            camPos *= m_cameraRadius;

            camPos += m_cameraFocusPoint;

            camLookAt = m_cameraFocusPoint;

            int windowWidth;
            int windowHeight;
            glfwGetWindowSize(m_glfwWindow, &windowWidth, &windowHeight);

            const float zNear = 0.1f;
            const float zFar = 1000.0f;
            const float fovy = 45.0f;
            const float aspect = (float) windowHeight / (float) windowWidth;
            camera.matProj.PerspectiveVulkan(fovy, aspect, zNear, zFar);
            camera.matProj = camera.matProj.Transpose();

            camera.matView.LookAt(camPos, camLookAt, camUp);
            camera.matView = camera.matView.Transpose();

            // Update the uniform buffer for the camera matrices
            memcpy(mappedData + uboByteOffset, &camera, sizeof(camera));

            cameraByteOFfset = uboByteOffset;

            // update offset into the buffer
            uboByteOffset += m_deviceContext.GetAligendUniformByteOffset(sizeof(camera));
        }

        //
        // Update the uniform buffer with the shadow camera information
        //
        {
            Vec3 camPos = Vec3(1, 1, 1) * 75.0f;
            Vec3 camLookAt = Vec3(0, 0, 0);
            Vec3 camUp = Vec3(0, 0, 1);
            Vec3 tmp = camPos.Cross(camUp);
            camUp = tmp.Cross(camPos);
            camUp.Normalize();

            extern FrameBuffer g_shadowFrameBuffer;
            const int windowWidth = g_shadowFrameBuffer.m_parms.width;
            const int windowHeight = g_shadowFrameBuffer.m_parms.height;

            const float halfWidth = 60.0f;
            const float xmin = -halfWidth;
            const float xmax = halfWidth;
            const float ymin = -halfWidth;
            const float ymax = halfWidth;
            const float zNear = 25.0f;
            const float zFar = 175.0f;
            camera.matProj.OrthoVulkan(xmin, xmax, ymin, ymax, zNear, zFar);
            camera.matProj = camera.matProj.Transpose();

            camera.matView.LookAt(camPos, camLookAt, camUp);
            camera.matView = camera.matView.Transpose();

            // Update the uniform buffer for the camera matrices
            memcpy(mappedData + uboByteOffset, &camera, sizeof(camera));

            shadowByteOffset = uboByteOffset;

            // update offset into the buffer
            uboByteOffset += m_deviceContext.GetAligendUniformByteOffset(sizeof(camera));
        }

        //
        //	Update the uniform buffer with the body positions/orientations
        //
        //for (int i = 0; i < m_scene->m_bodies.size(); i++)
        //{
        //    Body &body = m_scene->m_bodies[i];

        //    Vec3 fwd = body.m_orientation.RotatePoint(Vec3(1, 0, 0));
        //    Vec3 up = body.m_orientation.RotatePoint(Vec3(0, 0, 1));

        //    Mat4 matOrient;
        //    matOrient.Orient(body.m_position, fwd, up);
        //    matOrient = matOrient.Transpose();

        //    // Update the uniform buffer with the orientation of this body
        //    memcpy(mappedData + uboByteOffset, matOrient.ToPtr(), sizeof(matOrient));

        //    RenderModel renderModel;
        //    renderModel.model = m_models[i];
        //    renderModel.uboByteOffset = uboByteOffset;
        //    renderModel.uboByteSize = sizeof(matOrient);
        //    renderModel.pos = body.m_position;
        //    renderModel.orient = body.m_orientation;
        //    m_renderModels.push_back(renderModel);

        //    uboByteOffset += m_deviceContext.GetAligendUniformByteOffset(sizeof(matOrient));
        //}

        m_uniformBuffer.UnmapBuffer(&m_deviceContext);
    }
}

/*
====================================================
Application::DrawFrame
====================================================
*/
void Application::DrawFrame()
{
    UpdateUniforms();

    static int frameCount = 0;
    static float totalTime = 0.f;
    static auto lastTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    totalTime += deltaTime.count();
    frameCount++;
    static float fps = 30.f;
    if (totalTime >= 0.3f)
    {
        fps = frameCount / totalTime;

        frameCount = 0;
        totalTime = 0.0f;
    }

    //
    //	Begin the render frame
    //
    const uint32_t imageIndex = m_deviceContext.BeginFrame();

    // Draw everything in an offscreen buffer
    //DrawOffscreen(&m_deviceContext, imageIndex, &m_uniformBuffer, m_renderModels.data(), (int) m_renderModels.size());
    ElecNeko::DrawOffscreen(&m_deviceContext, imageIndex, &m_uniformBuffer, m_meshes);
    //
    //	Draw the offscreen framebuffer to the swap chain frame buffer
    //
    m_deviceContext.BeginRenderPass();
    {
        VkCommandBuffer cmdBuffer = m_deviceContext.m_vkCommandBuffers[imageIndex];
        {
            extern FrameBuffer g_offscreenFrameBuffer;

            // Binding the pipeline is effectively the "use shader" we had back in our opengl apps
            m_copyPipeline.BindPipeline(cmdBuffer);

            // Descriptor is how we bind our buffers and images
            Descriptor descriptor = m_copyPipeline.GetFreeDescriptor();
            descriptor.BindImage(VK_IMAGE_LAYOUT_GENERAL, g_offscreenFrameBuffer.m_imageColor.m_vkImageView,
                                 Samplers::m_samplerStandard, 0);
            descriptor.BindDescriptor(&m_deviceContext, cmdBuffer, &m_copyPipeline);
            m_modelFullScreen.DrawIndexed(cmdBuffer);
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Settings");
        ImGui::Text(m_deviceContext.m_physicalDevices[m_deviceContext.m_deviceIndex].m_vkDeviceProperties.deviceName);
        ImGui::Text("FPS: %.1f", fps);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
    }
    m_deviceContext.EndRenderPass();

    //
    //	End the render frame
    //
    m_deviceContext.EndFrame();
}
