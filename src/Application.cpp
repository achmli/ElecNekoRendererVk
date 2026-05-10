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

#include "Renderer/Scene/LegacyWorldConverter.h"
#include "Renderer/Scene/RenderScene.h"
#include "Scene.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

Application *g_application = NULL;

#include <time.h>
#include <windows.h>

static bool gIsInitialized(false);
static unsigned __int64 gTicksPerSecond;
static unsigned __int64 gStartTicks;

int sampleSceneIdx = 0;

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

namespace ElecNeko
{
    namespace fs = std::filesystem;
    static std::string ExtNormalized(const fs::path &p)
    {
        auto e = p.extension().string();
        if (!e.empty() && e.front() == '.')
        {
            e.erase(0, 1);
        }
        return e;
    }

    std::vector<fs::path> FindSceneFilesInDir(const fs::path &dir, bool throwOnError = false)
    {
        std::vector<fs::path> out;
        std::error_code ec;
        if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec))
        {
            if (throwOnError)
                throw std::runtime_error("not a directory: " + dir.string());
            return out;
        }

        for (fs::directory_iterator it(dir, ec); it != fs::directory_iterator(); it.increment(ec))
        {
            if (ec)
            {
                if (throwOnError)
                    throw fs::filesystem_error("directory iteration", dir, ec);
                break;
            }

            const fs::directory_entry &de = *it;
            if (!de.is_regular_file(ec))
            {
                continue;
            }
            if (ExtNormalized(de.path()) == "scene")
            {
                out.push_back(de.path());
            }
        }
        return out;
    }
} // namespace ElecNeko

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

    // m_scene = new Scene;
    // m_scene->Initialize();
    // m_scene->Reset();


    // m_camera.Initialize(Vec3(75.f, 75.f, 75.f), Vec3(32.5f, -6.6f, -49.3f), 80.f, static_cast<float>(WINDOW_HEIGHT) / static_cast<float>(WINDOW_WIDTH), .1f,
    // 1000.f);

    m_skyBox.LoadFromFile(&m_deviceContext, "Skybox");

    m_sceneFiles = ElecNeko::FindSceneFilesInDir("../res/scenes");


    world = new ElecNeko::World();
    world->LoadSceneFromFile(&m_deviceContext, m_sceneFiles[sampleSceneIdx].string());
    // for (auto &instance: world->m_meshInstances)
    // {
    //     instance.MakeUBO(&m_deviceContext);
    // }
    //
    // for (auto &mate: world->m_materials)
    // {
    //     mate.MakeBuffer(&m_deviceContext);
    // }

    world->CreateDefaultTextures(&m_deviceContext);
    for (auto *light: world->m_lights)
    {
        light->UpdateUBO(&m_deviceContext);
    }
    // m_scene = new ElecNeko::Scene();
    // m_scene->Initialize(&m_deviceContext, world);
    // m_scene->MakeVBO(&m_deviceContext);
    // m_scene->MakeUBO(&m_deviceContext);
    // m_scene = new ElecNeko::Scene();
    // m_scene->SetBuildLegacyOpaqueGeometry(false);
    // m_scene->Initialize(&m_deviceContext, world);
    // m_scene->MakeVBO(&m_deviceContext);
    // m_scene->MakeUBO(&m_deviceContext);
    m_scene = new ElecNeko::Scene;

    m_scene->SetBuildLegacyOpaqueGeometry(false);
    m_scene->SetBuildLegacyMaskedGeometry(false);
    m_scene->SetBuildLegacyTransparentGeometry(false);

    m_scene->Initialize(&m_deviceContext, world);
    m_scene->MakeVBO(&m_deviceContext);
    m_scene->MakeUBO(&m_deviceContext);

    printf("[Legacy Scene] opaqueVerts=%zu opaqueIdx=%zu maskVerts=%zu maskIdx=%zu transparentVerts=%zu transparentIdx=%zu materials=%zu matrices=%zu "
           "textureArray=%p\n",
           m_scene->opaqueVertices.size(), m_scene->opaqueIndices.size(), m_scene->maskVertices.size(), m_scene->maskIndices.size(),
           m_scene->transparentVertices.size(), m_scene->transparentIndices.size(), m_scene->materials.size(), m_scene->modelMatrices.size(),
           static_cast<void *>(m_scene->textureArray));

    m_renderScene = ElecNeko::ConvertLegacyWorldToRenderScene(&m_deviceContext, world).release();

    printf("[RenderScene] meshes=%zu instances=%zu opaque=%zu masked=%zu transparent=%zu shadow=%zu gpuInstances=%zu gpuMaterials=%zu instanceBuffer=%llu "
           "materialBuffer=%llu\n",
           m_renderScene->meshes.size(), m_renderScene->meshInstances.size(), m_renderScene->drawList.opaque.size(), m_renderScene->drawList.masked.size(),
           m_renderScene->drawList.transparent.size(), m_renderScene->drawList.shadow.size(), m_renderScene->gpuInstances.size(),
           m_renderScene->gpuMaterials.size(), static_cast<unsigned long long>(m_renderScene->instanceBuffer.m_vkBufferSize),
           static_cast<unsigned long long>(m_renderScene->materialBuffer.m_vkBufferSize));

    m_shadowCamera.Initialize(world->m_cam->position + Vec3(renderOption.sunDirection) * 10, world->m_cam->position, static_cast<float>(WINDOW_WIDTH),
                              static_cast<float>(WINDOW_HEIGHT), 25, 175);

    m_csm.Initialize(&m_deviceContext);

    m_mousePosition = Vec2(0, 0);

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
    if (VK_SUCCESS != glfwCreateWindowSurface(m_deviceContext.m_vkInstance, m_glfwWindow, nullptr, &m_deviceContext.m_vkSurface))
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
    ElecNeko::ElecNekoSampler::InitializeSampler(&m_deviceContext);

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
    ElecNeko::InitOffscreen(&m_deviceContext, renderOption, m_deviceContext.m_swapChain.m_windowWidth, m_deviceContext.m_swapChain.m_windowHeight);

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

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_deviceContext.m_swapChain.m_vkColorImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = m_deviceContext.m_swapChain.m_vkDepthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[2] = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

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
    initInfo.MinImageCount = m_deviceContext.m_physicalDevices[m_deviceContext.m_deviceIndex].m_vkSurfaceCapabilities.minImageCount;
    initInfo.ImageCount = m_deviceContext.m_physicalDevices[m_deviceContext.m_deviceIndex].m_vkSurfaceCapabilities.minImageCount + 1;
    initInfo.CheckVkResultFn = nullptr;
    initInfo.RenderPass = m_imguiRenderPass;
    ImGui_ImplVulkan_Init(&initInfo);

    // VkCommandBuffer cmdBuffer = m_deviceContext.BeginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture();
}


/*
====================================================
Application::Cleanup
====================================================
*/
void Application::Cleanup()
{
    ElecNeko::CleanupOffscreen(&m_deviceContext, renderOption);

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
    // delete m_scene;
    // m_scene = NULL;

    // Delete models
    /*for (int i = 0; i < m_models.size(); i++)
    {
        m_models[i]->Cleanup(m_deviceContext);
        delete m_models[i];
    }
    m_models.clear();*/

    m_skyBox.Cleanup(&m_deviceContext);

    if (m_renderScene != nullptr)
    {
        m_renderScene->Cleanup(&m_deviceContext);
        delete m_renderScene;
        m_renderScene = nullptr;
    }

    m_scene->Cleanup(&m_deviceContext);
    delete m_scene;

    world->LightClean(&m_deviceContext);
    delete world;

    m_csm.Cleanup(&m_deviceContext);

    // Delete Uniform Buffer Memory
    m_uniformBuffer.Cleanup(&m_deviceContext);

    // Delete Samplers
    Samplers::Cleanup(&m_deviceContext);
    ElecNeko::ElecNekoSampler::Cleanup(&m_deviceContext);

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
    // if (application->m_isMouseDown)
    // application->MouseMoved((float) x, (float) y);

    if (application->m_isRightButtonDown)
    {
        application->MouseMoved((float) x, (float) y);
    }
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

    world->m_cam->OffsetOrientation(ds.x, ds.y);
}

void Application::LeftMouseMoved(float x, float y)
{
    Vec2 newPosition = Vec2(x, y);
    Vec2 ds = newPosition - m_mousePosition;
    m_mousePosition = newPosition;

    m_shadowCamera.OffsetOrientation(ds.x, ds.y);
}

void Application::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    Application *application = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            application->m_isMouseDown = true;

            double x, y;
            glfwGetCursorPos(window, &x, &y);
            application->m_mousePosition = Vec2(static_cast<float>(x), static_cast<float>(y));
        }
        else if (action == GLFW_RELEASE)
        {
            application->m_isMouseDown = false;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            application->m_isRightButtonDown = true;

            double x, y;
            glfwGetCursorPos(window, &x, &y);
            application->m_mousePosition = Vec2(static_cast<float>(x), static_cast<float>(y));
        }
        else if (action == GLFW_RELEASE)
        {
            application->m_isRightButtonDown = false;
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
void Application::MouseScrolled(float z) { world->m_cam->position += world->m_cam->forward * z * m_cameraMoveSpeed; }

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
        // m_scene->Reset();
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

void Application::ProcessKeyboard(float deltaTime)
{
    float velocity = m_cameraMoveSpeed * deltaTime;

    if (glfwGetKey(m_glfwWindow, GLFW_KEY_W) == GLFW_PRESS)
        world->m_cam->position += world->m_cam->forward * velocity;
    if (glfwGetKey(m_glfwWindow, GLFW_KEY_S) == GLFW_PRESS)
        world->m_cam->position -= world->m_cam->forward * velocity;
    if (glfwGetKey(m_glfwWindow, GLFW_KEY_A) == GLFW_PRESS)
        world->m_cam->position -= world->m_cam->right * velocity;
    if (glfwGetKey(m_glfwWindow, GLFW_KEY_D) == GLFW_PRESS)
        world->m_cam->position += world->m_cam->right * velocity;
    if (glfwGetKey(m_glfwWindow, GLFW_KEY_SPACE) == GLFW_PRESS)
        world->m_cam->position += world->m_cam->up * velocity;
    if (glfwGetKey(m_glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        world->m_cam->position -= world->m_cam->up * velocity;
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
        /* if (dt_us < 16000.0f)
         {
             int x = 16000 - (int) dt_us;
             std::this_thread::sleep_for(std::chrono::microseconds(x));
             dt_us = 16000;
             time = GetTimeMicroseconds();
         }
        timeLastFrame = time;*/
        // long long time = GetTimeMicroseconds();
        // float dt_us = float(time - timeLastFrame);

        // if (dt_us < 16000.0f)
        //{
        //     auto sleepTime = std::chrono::microseconds((int) (16000 - dt_us));
        //     std::this_thread::sleep_for(sleepTime);
        //     time = GetTimeMicroseconds(); // ��ȡһ����ʵʱ��
        //     dt_us = float(time - timeLastFrame);
        // }

        // timeLastFrame = time;
        // printf("\ndt_ms: %.1f    ", dt_us * 0.001f);

        // Get User Input
        glfwPollEvents();

        int currentWidth = 0, currentHeight = 0;
        glfwGetFramebufferSize(m_glfwWindow, &currentWidth, &currentHeight);

        if (currentWidth == 0 || currentHeight == 0)
        {
            // stop rendering when window minimalized
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

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
                // m_scene->Update(dt_sec * 0.5f);
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

        if (deletingWorld && deletingScene && deletingRenderScene)
        {
            /*int deletingNums = m_toDeleteMeshes.size();
            for (int i = 0; i < deletingNums; i++)
            {
                m_toDeleteMeshes[i].DeferedCleanup(&m_deviceContext);

                if (m_toDeleteMeshes[i].currentLoop > m_toDeleteMeshes[i].loopTime)
                {
                    if (i < (deletingNums - 1))
                    {
                        std::swap(m_toDeleteMeshes[i], m_toDeleteMeshes[deletingNums - 1]);
                    }

                    m_toDeleteMeshes.pop_back();
                }
            }*/
            static int currentLoop = 0;
            int totalLoops = 100;
            if (currentLoop < totalLoops)
            {
                currentLoop++;
            }
            else
            {
                deletingRenderScene->Cleanup(&m_deviceContext);
                delete deletingRenderScene;
                deletingRenderScene = nullptr;

                deletingScene->Cleanup(&m_deviceContext);
                delete deletingScene;
                deletingScene = nullptr;

                deletingWorld->LightClean(&m_deviceContext);
                delete deletingWorld;
                deletingWorld = nullptr;

                currentLoop = 0;
            }
        }

        m_shadowCamera.UpdateCamera(world->m_cam->position, renderOption.sunDirection);

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
    uint32_t lightParmsByteOffset = 0;
    uint32_t skyParmsByteOffset = 0;

    struct camera_t
    {
        Mat4 matView;
        Mat4 matProj;
        Mat4 viewNoTrans;
        Mat4 invView;
        Mat4 invProj;
    };
    camera_t camera;

    struct lightParms_t
    {
        float viewPort[2];
        uint32_t tonemap;
        uint32_t enableExplosure;

        float sunDir[3];
        float sunIntensity;

        float sunColor[3];
        float sunAngularRadius;

        float sunGlowSpread;
        float explosure;
        uint32_t aces;
        uint32_t simpleAces;
    };
    lightParms_t lightParms;

    struct skyParms_t
    {
        float skyColor[3];
        float zenithBrightenA;
        float horizonBrightenB;
        float baseSkyBrightnessC;
        float exponentialScatteringD;
        float circumsolarGlowE;
        float angularScatteringF;
        float HG;
        float HGParmH;
        float horizonFalloffI;
        float lm;
        float cameraPos[3];
    } skyParms;

    //
    //	Update the uniform buffers
    //
    {
        unsigned char *mappedData = (unsigned char *) m_uniformBuffer.MapBuffer(&m_deviceContext);

        //
        // Update the uniform buffer with the camera information
        //
        {
            /*camera.matProj = m_camera.ComputeProjectionMatrix();
            camera.matView = m_camera.ComputeViewMatrix();

            camera.viewNoTrans = camera.matView;
            camera.viewNoTrans.rows[0].w = 0.0f;
            camera.viewNoTrans.rows[1].w = 0.0f;
            camera.viewNoTrans.rows[2].w = 0.0f;
            camera.viewNoTrans.rows[3] = Vec4(0, 0, 0, 1);

            camera.invView = camera.matView.Inverse();
            camera.invProj = camera.matProj.Inverse();*/

            camera.matProj = world->m_cam->ComputeProjectionMatrix();
            camera.matView = world->m_cam->ComputeViewMatrix();


            camera.viewNoTrans = camera.matView;
            camera.viewNoTrans.rows[0].w = 0.0f;
            camera.viewNoTrans.rows[1].w = 0.0f;
            camera.viewNoTrans.rows[2].w = 0.0f;
            camera.viewNoTrans.rows[3] = Vec4(0, 0, 0, 1);

            camera.invView = camera.matView.Inverse();
            camera.invProj = camera.matProj.Inverse();

            // Update the uniform buffer for the camera matrices
            memcpy(mappedData + uboByteOffset, &camera, sizeof(camera));

            cameraByteOFfset = uboByteOffset;

            // update offset into the buffer
            uboByteOffset += m_deviceContext.GetAligendUniformByteOffset(sizeof(camera));

            m_csm.UpdateMainView(camera.matView, camera.invView, world->m_cam->fov);
        }

        //
        // Update the uniform buffer with the shadow camera information
        //
        {
            /*m_shadowCamera.forward = Vec3(0 - renderOption.sunDirection[0] * 30, 0 - renderOption.sunDirection[1] * 30, 0 - renderOption.sunDirection[2] *
            30); m_shadowCamera.position = m_camera.position + Vec3(renderOption.sunDirection) * 30;*/

            camera.matView = m_shadowCamera.ComputeViewMatrix();
            camera.matProj = m_shadowCamera.ComputeProjctionMatrix();

            camera.viewNoTrans = camera.matView;
            camera.viewNoTrans.rows[0].w = 0.0f;
            camera.viewNoTrans.rows[1].w = 0.0f;
            camera.viewNoTrans.rows[2].w = 0.0f;
            camera.viewNoTrans.rows[3] = Vec4(0, 0, 0, 1);

            camera.invView = camera.matView.Inverse();
            camera.invProj = camera.matProj.Inverse();

            // Update the uniform buffer for the camera matrices
            memcpy(mappedData + uboByteOffset, &camera, sizeof(camera));

            shadowByteOffset = uboByteOffset;

            // update offset into the buffer
            uboByteOffset += m_deviceContext.GetAligendUniformByteOffset(sizeof(camera));
        }

        {
            lightParms.viewPort[0] = WINDOW_WIDTH;
            lightParms.viewPort[1] = WINDOW_HEIGHT;

            lightParms.tonemap = renderOption.tonemapping ? 1 : 0;
            lightParms.enableExplosure = renderOption.enableExplosure ? 1 : 0;

            lightParms.sunDir[0] = renderOption.sunDirection[0];
            lightParms.sunDir[1] = renderOption.sunDirection[1];
            lightParms.sunDir[2] = renderOption.sunDirection[2];

            lightParms.sunIntensity = renderOption.sunIntensity;

            lightParms.sunColor[0] = renderOption.sunColor[0];
            lightParms.sunColor[1] = renderOption.sunColor[1];
            lightParms.sunColor[2] = renderOption.sunColor[2];

            lightParms.sunAngularRadius = ElecNeko::Radians(renderOption.sunAngularRadius);
            lightParms.sunGlowSpread = renderOption.sunGlowSpread;
            lightParms.explosure = renderOption.explosure;

            lightParms.aces = renderOption.ACESFit ? 1 : 0;
            lightParms.simpleAces = renderOption.simpleACESFit ? 1 : 0;

            memcpy(mappedData + uboByteOffset, &lightParms, sizeof(lightParms));

            lightParmsByteOffset = uboByteOffset;

            uboByteOffset += m_deviceContext.GetAligendUniformByteOffset(sizeof(lightParms));
        }

        {
            skyParms.skyColor[0] = renderOption.skyColor[0];
            skyParms.skyColor[1] = renderOption.skyColor[1];
            skyParms.skyColor[2] = renderOption.skyColor[2];

            skyParms.zenithBrightenA = renderOption.expStrengthA;
            skyParms.horizonBrightenB = renderOption.expAAttenuationB;
            skyParms.baseSkyBrightnessC = renderOption.baseConstantC;
            skyParms.exponentialScatteringD = renderOption.expGammaAttenuationD;
            skyParms.circumsolarGlowE = renderOption.expAttenuationSpeedE;
            skyParms.angularScatteringF = renderOption.gammaScatteringF;
            skyParms.HG = renderOption.chiContributeG;
            skyParms.HGParmH = renderOption.chiParmH;
            skyParms.horizonFalloffI = renderOption.thetaFixI;
            skyParms.lm = renderOption.Lm;

            skyParms.cameraPos[0] = world->m_cam->position.x;
            skyParms.cameraPos[1] = world->m_cam->position.y;
            skyParms.cameraPos[2] = world->m_cam->position.z;

            memcpy(mappedData + uboByteOffset, &skyParms, sizeof(skyParms));

            skyParmsByteOffset = uboByteOffset;

            uboByteOffset += m_deviceContext.GetAligendUniformByteOffset(sizeof(skyParms));
        }

        //
        //	Update the uniform buffer with the body positions/orientations
        //
        // for (int i = 0; i < m_scene->m_bodies.size(); i++)
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

    ProcessKeyboard(deltaTime.count());
    UpdateUniforms();
    //
    //	Begin the render frame
    //
    const uint32_t imageIndex = m_deviceContext.BeginFrame();

    // Draw everything in an offscreen buffer
    // DrawOffscreen(&m_deviceContext, imageIndex, &m_uniformBuffer, m_renderModels.data(), (int) m_renderModels.size());
    // ElecNeko::DrawOffscreen(&m_deviceContext, imageIndex, &m_uniformBuffer, m_skyBox, m_meshes, renderOption);
    // ElecNeko::DrawOffscreen(&m_deviceContext, imageIndex, &m_uniformBuffer, m_skyBox, m_scene, renderOption, m_csm);
    ElecNeko::DrawOffscreen(&m_deviceContext, imageIndex, &m_uniformBuffer, m_skyBox, m_scene, renderOption, m_csm, m_renderScene);
    //
    //	Draw the offscreen framebuffer to the swap chain frame buffer
    //
    m_deviceContext.BeginRenderPass();
    {
        extern FrameBuffer g_postProcessFrameBuffer;
        extern Image c_postProcessImage;
        VkCommandBuffer cmdBuffer = m_deviceContext.m_vkCommandBuffers[imageIndex];
        {
            // Binding the pipeline is effectively the "use shader" we had back in our opengl apps
            m_copyPipeline.BindPipeline(cmdBuffer);

            // Descriptor is how we bind our buffers and images
            Descriptor descriptor = m_copyPipeline.GetFreeDescriptor();
            if (!renderOption.isDeferred)
                descriptor.BindImage(VK_IMAGE_LAYOUT_GENERAL, g_postProcessFrameBuffer.m_imageColor.m_vkImageView, Samplers::m_samplerStandard, 0);
            else
                descriptor.BindImage(VK_IMAGE_LAYOUT_GENERAL, c_postProcessImage.m_vkImageView, Samplers::m_samplerStandard, 0);
            descriptor.BindDescriptor(&m_deviceContext, cmdBuffer, &m_copyPipeline);
            m_modelFullScreen.DrawIndexed(cmdBuffer);
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Settings");
        ImGui::Text(m_deviceContext.m_physicalDevices[m_deviceContext.m_deviceIndex].m_vkDeviceProperties.deviceName);
        ImGui::Text("FPS: %.1f", fps);

        static std::string sceneName = m_sceneFiles[sampleSceneIdx].string();

        auto getter = [](void *data, int idx, const char **outText) -> bool
        {
            auto const &vec = *static_cast<const std::vector<std::filesystem::path> *>(data);
            if (idx < 0 || idx >= static_cast<int>(vec.size()))
            {
                return false;
            }
            static std::string tmp;
            tmp = vec[idx].filename().string();
            *outText = tmp.c_str();
            return true;
        };

        if (ImGui::Combo("Scene", &sampleSceneIdx, getter, (void *) &m_sceneFiles, m_sceneFiles.size()))
        {
            if (sceneName != m_sceneFiles[sampleSceneIdx])
            {
                sceneName = m_sceneFiles[sampleSceneIdx].string();

                // deletingWorld = world;
                // deletingScene = m_scene;
                // world = new ElecNeko::World;
                // world->LoadSceneFromFile(&m_deviceContext, sceneName);
                // world->CreateDefaultTextures(&m_deviceContext);
                deletingWorld = world;
                deletingScene = m_scene;
                deletingRenderScene = m_renderScene;

                world = new ElecNeko::World;
                world->LoadSceneFromFile(&m_deviceContext, sceneName);
                world->CreateDefaultTextures(&m_deviceContext);
                // for (auto &instance: world->m_meshInstances)
                // {
                //     instance.MakeUBO(&m_deviceContext);
                // }
                //
                // for (auto &mate: world->m_materials)
                // {
                //     mate.MakeBuffer(&m_deviceContext);
                // }
                // m_scene = new ElecNeko::Scene;
                // m_scene->Initialize(&m_deviceContext, world);
                // m_scene->MakeVBO(&m_deviceContext);
                // m_scene->MakeUBO(&m_deviceContext);
                // m_scene = new ElecNeko::Scene;
                // m_scene->SetBuildLegacyOpaqueGeometry(false);
                // m_scene->Initialize(&m_deviceContext, world);
                // m_scene->MakeVBO(&m_deviceContext);
                // m_scene->MakeUBO(&m_deviceContext);
                m_scene = new ElecNeko::Scene();

                m_scene->SetBuildLegacyOpaqueGeometry(false);
                m_scene->SetBuildLegacyMaskedGeometry(false);
                m_scene->SetBuildLegacyTransparentGeometry(false);

                m_scene->Initialize(&m_deviceContext, world);
                m_scene->MakeVBO(&m_deviceContext);
                m_scene->MakeUBO(&m_deviceContext);


                printf("[Legacy Scene] opaqueVerts=%zu opaqueIdx=%zu maskVerts=%zu maskIdx=%zu transparentVerts=%zu transparentIdx=%zu materials=%zu "
                       "matrices=%zu "
                       "textureArray=%p\n",
                       m_scene->opaqueVertices.size(), m_scene->opaqueIndices.size(), m_scene->maskVertices.size(), m_scene->maskIndices.size(),
                       m_scene->transparentVertices.size(), m_scene->transparentIndices.size(), m_scene->materials.size(), m_scene->modelMatrices.size(),
                       static_cast<void *>(m_scene->textureArray));

                m_renderScene = ElecNeko::ConvertLegacyWorldToRenderScene(&m_deviceContext, world).release();

                printf("[RenderScene] meshes=%zu instances=%zu opaque=%zu masked=%zu transparent=%zu shadow=%zu gpuInstances=%zu gpuMaterials=%zu "
                       "instanceBuffer=%llu materialBuffer=%llu\n",
                       m_renderScene->meshes.size(), m_renderScene->meshInstances.size(), m_renderScene->drawList.opaque.size(),
                       m_renderScene->drawList.masked.size(), m_renderScene->drawList.transparent.size(), m_renderScene->drawList.shadow.size(),
                       m_renderScene->gpuInstances.size(), m_renderScene->gpuMaterials.size(),
                       static_cast<unsigned long long>(m_renderScene->instanceBuffer.m_vkBufferSize),
                       static_cast<unsigned long long>(m_renderScene->materialBuffer.m_vkBufferSize));
            }
        }
        ImGui::Checkbox("Deferred Rendering", &renderOption.isDeferred);

        if (ImGui::CollapsingHeader("Sky"))
        {
            ImGui::Checkbox("SkyBox", &renderOption.skyBox);
            ImGui::Checkbox("SimpleRealSky", &renderOption.simpleRealSky);
        }

        if (renderOption.simpleRealSky)
        {
            if (ImGui::CollapsingHeader("Sun Parameters"))
            {
                if (ImGui::ColorEdit3("SunColor", renderOption.sunColor, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::DragFloat3("SunDirection", renderOption.sunDirection, 0.01f, -1.f, 1.f))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("SunIntensity", &renderOption.sunIntensity, 1.f, 20.f))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("SunAngularRadius", &renderOption.sunAngularRadius, .28f, 1.14f))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("SunGlowSpread", &renderOption.sunGlowSpread, 0.02f, 0.12f))
                {
                    renderOption.isSkyChanged = true;
                }
                ImGui::SliderFloat("Explosure", &renderOption.explosure, 0.8f, 1.6f);
            }

            if (ImGui::CollapsingHeader("Sky Parameters"))
            {
                if (ImGui::ColorEdit3("SkyColor", renderOption.skyColor, ImGuiColorEditFlags_Float))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("ZenithBrighten", &renderOption.expStrengthA, 0, 10.f))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("HorizonBrighten", &renderOption.expAAttenuationB, -1.f, -.2f))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("BaseSkyBrightness", &renderOption.baseConstantC, .1f, 1.f))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("ExponentialScattering", &renderOption.expGammaAttenuationD, 0.05f, .5f))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("CircumsolarGlow", &renderOption.expAttenuationSpeedE, -5.f, -1.f))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("AngularScattering", &renderOption.gammaScatteringF, 0.01f, 0.2f))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("HenyeyGreenstein", &renderOption.chiContributeG, 0.01f, 0.1f))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("HGParm", &renderOption.chiParmH, .5f, .9f))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("HorizonFalloff", &renderOption.thetaFixI, .05f, .2f))
                {
                    renderOption.isSkyChanged = true;
                }
                if (ImGui::SliderFloat("ReferenceRadiance", &renderOption.Lm, 1.f, 20.f))
                {
                    renderOption.isSkyChanged = true;
                }
            }
        }

        if (ImGui::CollapsingHeader("Post Process"))
        {
            ImGui::Checkbox("Enable Tonemap", &renderOption.tonemapping);
            if (renderOption.tonemapping)
            {
                if (ImGui::CollapsingHeader("Tonemapping"))
                {
                    ImGui::Checkbox("enableExplosure", &renderOption.enableExplosure);
                    ImGui::Checkbox("ACES Fit", &renderOption.ACESFit);
                    if (renderOption.ACESFit)
                    {
                        ImGui::Checkbox("Simple ACES Fit", &renderOption.simpleACESFit);
                    }
                }
            }
        }

        if (ImGui::CollapsingHeader("Ambient Occlusion"))
        {
            ImGui::Checkbox("Use Ambient Occlusion", &renderOption.useAO);
            ImGui::Checkbox("Use Contact Shadow", &renderOption.useCT);
        }

        if (ImGui::CollapsingHeader("Shadow"))
        {
            ImGui::SliderFloat("uniform to logarithmic", &m_csm.lambda, 0.0, 1.0);
            ImGui::SliderFloat("kNear", &m_csm.mergeN, 0.5f, 10.0f);
            ImGui::SliderFloat("kFar", &m_csm.mergeF, 0.5f, 10.0f);
            ImGui::Checkbox("Visualize Cascade", &m_csm.visualizeCascade);
            ImGui::Checkbox("Stabilize Shadow", &m_csm.stabilizeTexels);
        }

        if (ImGui::CollapsingHeader("SSR"))
        {
            ImGui::SliderInt("max steps", &renderOption.maxSSRSteps, 1, 256);
            ImGui::SliderFloat("max distance", &renderOption.maxSSRDistance, 5.f, 200.f);
            ImGui::SliderFloat("stride scale", &renderOption.strideSSRScale, 0.1f, 4.f);
            ImGui::SliderFloat("thickness", &renderOption.thicknessSSR, 1e-4f, .2f);
            ImGui::SliderInt("binary iterations", &renderOption.binarySearchSSRIters, 1, 8);
            ImGui::SliderFloat("roughness threshold", &renderOption.roughnessSSREnabled, 0.f, 1.f);
            ImGui::SliderFloat("SSR Strength", &renderOption.ssrStrength, 0.5f, 2.f);
            ImGui::SliderFloat("IBL Intensity", &renderOption.envIntensity, 0.1f, 1.f);
        }

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
