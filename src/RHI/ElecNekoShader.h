#pragma once

#include <array>
#include <string>
#include <vector>

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.h>

#include "DeviceContext.h"

namespace ElecNeko
{
    class ElecNekoShader
    {
    public:
        enum ShaderStage_t
        {
            SHADER_STAGE_VERTEX = 0,
            SHADER_STAGE_TESSELLATION_CONTROL,
            SHADER_STAGE_TESSELLATION_EVALUATION,
            SHADER_STAGE_GEOMETRY,
            SHADER_STAGE_FRAGMENT,
            SHADER_STAGE_COMPUTE,
            SHADER_STAGE_RAYGEN,
            SHADER_STAGE_ANY_HIT,
            SHADER_STAGE_CLOSEST_HIT,
            SHADER_STAGE_MISS,
            SHADER_STAGE_INTERSECTION,
            SHADER_STAGE_CALLABLE,
            SHADER_STAGE_TASK,
            SHADER_STAGE_MESH,
            SHADER_STAGE_COUNT
        };

        ElecNekoShader() noexcept;
        ~ElecNekoShader() {}

        bool Load(DeviceContext *device, const char *name, bool writeSpvToDisk = true);
        void Cleanup(DeviceContext *device);

        VkShaderModule GetModule(ShaderStage_t stage) const { return m_vkShaderModules[stage]; }

    public:
        static VkShaderModule CreateShaderModule(VkDevice vkDevice, const uint32_t *code, size_t wordCount);
        static VkShaderModule CreateShaderModuleFromBytes(VkDevice vkDevice, const std::vector<uint8_t> &bytes);

        static int ShaderStageToShaderCKind(ShaderStage_t s);
        static const char *StageExtension(ShaderStage_t s);
        static const char *StageVulkanName(ShaderStage_t s);

    public:
        std::array<VkShaderModule, SHADER_STAGE_COUNT> m_vkShaderModules;
    };
} // namespace ElecNeko
