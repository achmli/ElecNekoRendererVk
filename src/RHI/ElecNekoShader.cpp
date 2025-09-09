#include "ElecNekoShader.h"
#include "Loader/FileioModern.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>

#if USE_SHADERC
#include <shaderc/shaderc.hpp>
#endif

namespace ElecNeko
{
    class FileIncluder : public shaderc::CompileOptions::IncluderInterface
    {
    public:
        explicit FileIncluder(const std::filesystem::path &base) : m_base(base) {}
        shaderc_include_result *GetInclude(const char *requested_source, shaderc_include_type type, const char *requesting_source,
                                           size_t include_depth) override
        {
            std::filesystem::path resolved;
            std::string req(requested_source ? requested_source : "");
            if (type == shaderc_include_type_relative && requesting_source && requesting_source[0] != '\0')
            {
                std::filesystem::path requester(requesting_source);
                resolved = (requester.parent_path() / req).lexically_normal();
            }
            else
            {
                resolved = (m_base / req).lexically_normal();
            }

            auto textOpt = ReadFileText(resolved);
            shaderc_include_result *res = new shaderc_include_result();
            if (!textOpt)
            {
                std::string name = resolved.string();
                res->source_name = _strdup(name.c_str());
                res->source_name_length = name.size();
                res->content = nullptr;
                res->content_length = 0;
                res->user_data = nullptr;
                return res;
            }

            // allocate content storage
            auto *content = new std::string(std::move(*textOpt));
            res->source_name = _strdup(resolved.string().c_str());
            res->source_name_length = std::strlen(res->source_name);
            res->content = content->c_str();
            res->content_length = content->size();
            res->user_data = static_cast<void *>(content);
            return res;
        }

        void ReleaseInclude(shaderc_include_result *data) override
        {
            if (!data)
            {
                return;
            }
            if (data->user_data)
            {
                std::string *p = static_cast<std::string *>(data->user_data);
                delete p;
            }

            if (data->source_name)
            {
                free(const_cast<char *>(data->source_name));
            }
            delete data;
        }

    private:
        std::filesystem::path m_base;
    };

    ElecNekoShader::ElecNekoShader() noexcept { m_vkShaderModules.fill(VK_NULL_HANDLE); }

    void ElecNekoShader::Cleanup(DeviceContext *device)
    {
        for (int i = 0; i < SHADER_STAGE_COUNT; i++)
        {
            if (m_vkShaderModules[i] != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(device->m_vkDevice, m_vkShaderModules[i], nullptr);
                m_vkShaderModules[i] = VK_NULL_HANDLE;
            }
        }
    }

    int ElecNekoShader::ShaderStageToShaderCKind(ShaderStage_t s)
    {
        switch (s)
        {
            case SHADER_STAGE_VERTEX:
                return shaderc_glsl_vertex_shader;
            case SHADER_STAGE_TESSELLATION_CONTROL:
                return shaderc_glsl_tess_control_shader;
            case SHADER_STAGE_TESSELLATION_EVALUATION:
                return shaderc_glsl_tess_evaluation_shader;
            case SHADER_STAGE_GEOMETRY:
                return shaderc_glsl_geometry_shader;
            case SHADER_STAGE_FRAGMENT:
                return shaderc_glsl_fragment_shader;
            case SHADER_STAGE_COMPUTE:
                return shaderc_glsl_compute_shader;
            case SHADER_STAGE_RAYGEN:
                return shaderc_glsl_raygen_shader;
            case SHADER_STAGE_ANY_HIT:
                return shaderc_glsl_anyhit_shader;
            case SHADER_STAGE_CLOSEST_HIT:
                return shaderc_glsl_closesthit_shader;
            case SHADER_STAGE_MISS:
                return shaderc_glsl_miss_shader;
            case SHADER_STAGE_INTERSECTION:
                return shaderc_glsl_intersection_shader;
            case SHADER_STAGE_CALLABLE:
                return shaderc_glsl_callable_shader;
            case SHADER_STAGE_TASK:
                return shaderc_glsl_task_shader;
            case SHADER_STAGE_MESH:
                return shaderc_glsl_mesh_shader;
            default:
                return shaderc_glsl_vertex_shader;
        }
    }

    const char *ElecNekoShader::StageExtension(ShaderStage_t s)
    {
        switch (s)
        {
            case SHADER_STAGE_VERTEX:
                return "vert";
            case SHADER_STAGE_TESSELLATION_CONTROL:
                return "tesc";
            case SHADER_STAGE_TESSELLATION_EVALUATION:
                return "tese";
            case SHADER_STAGE_GEOMETRY:
                return "geom";
            case SHADER_STAGE_FRAGMENT:
                return "frag";
            case SHADER_STAGE_COMPUTE:
                return "comp";
            case SHADER_STAGE_RAYGEN:
                return "rgen";
            case SHADER_STAGE_ANY_HIT:
                return "ahit";
            case SHADER_STAGE_CLOSEST_HIT:
                return "chit";
            case SHADER_STAGE_MISS:
                return "miss";
            case SHADER_STAGE_INTERSECTION:
                return "rint";
            case SHADER_STAGE_CALLABLE:
                return "call";
            case SHADER_STAGE_TASK:
                return "task";
            case SHADER_STAGE_MESH:
                return "mesh";
            default:
                return "vert";
        }
    }

    const char *ElecNekoShader::StageVulkanName(ShaderStage_t s)
    {
        switch (s)
        {
            case SHADER_STAGE_VERTEX:
                return "vertex";
            case SHADER_STAGE_TESSELLATION_CONTROL:
                return "tesscontrol";
            case SHADER_STAGE_TESSELLATION_EVALUATION:
                return "tesseval";
            case SHADER_STAGE_GEOMETRY:
                return "geometry";
            case SHADER_STAGE_FRAGMENT:
                return "fragment";
            case SHADER_STAGE_COMPUTE:
                return "compute";
            case SHADER_STAGE_RAYGEN:
                return "raygen";
            case SHADER_STAGE_ANY_HIT:
                return "anyhit";
            case SHADER_STAGE_CLOSEST_HIT:
                return "closesthit";
            case SHADER_STAGE_MISS:
                return "miss";
            case SHADER_STAGE_INTERSECTION:
                return "intersection";
            case SHADER_STAGE_CALLABLE:
                return "callable";
            case SHADER_STAGE_TASK:
                return "task";
            case SHADER_STAGE_MESH:
                return "mesh";
            default:
                return "unknown";
        }
    }

    VkShaderModule ElecNekoShader::CreateShaderModule(VkDevice vkDevice, const uint32_t *code, size_t wordCount)
    {
        if (!code || wordCount == 0)
        {
            return VK_NULL_HANDLE;
        }
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = wordCount * sizeof(uint32_t);
        createInfo.pCode = code;
        VkShaderModule module;
        VkResult r = vkCreateShaderModule(vkDevice, &createInfo, nullptr, &module);
        if (r != VK_SUCCESS)
        {
            std::cerr << "Failed to create shader module\n";
            return VK_NULL_HANDLE;
        }
        return module;
    }

    VkShaderModule ElecNekoShader::CreateShaderModuleFromBytes(VkDevice vkDevice, const std::vector<uint8_t> &bytes)
    {
        if (bytes.empty())
        {
            return VK_NULL_HANDLE;
        }

        // byte length must be multiple of 4
        if (bytes.size() % 4 != 0)
        {
            std::cerr << "spirv byte size is not multiple of 4\n";
            return VK_NULL_HANDLE;
        }

        const uint32_t *code = reinterpret_cast<const uint32_t *>(bytes.data());
        size_t wordCount = bytes.size() / sizeof(uint32_t);
        return CreateShaderModule(vkDevice, code, wordCount);
    }

    bool ElecNekoShader::Load(DeviceContext *device, const char *name, bool writeSpvToDisk)
    {
        if (!device || !name)
        {
            return false;
        }

        // clear/initialize modules
        for (int i = 0; i < SHADER_STAGE_COUNT; ++i)
        {
            m_vkShaderModules[i] = VK_NULL_HANDLE;
        }

        shaderc::Compiler compiler;

        std::filesystem::path spirvDir = std::filesystem::path("../src/shaders/spirv");
        std::filesystem::path srcDir = std::filesystem::path("../src/shaders");

        for (int s = 0; s < SHADER_STAGE_COUNT; s++)
        {
            const char *ext = StageExtension((ShaderStage_t) s);
            std::string spirvPathStr = (spirvDir / (std::string(name) + "." + ext + ".spirv")).generic_string();
            std::string srcPathStr = (srcDir / (std::string(name) + "." + ext)).generic_string();

            // check existence
            bool spirvExists = FileExists(spirvPathStr);
            bool srcExists = FileExists(srcPathStr);

            // If SPIR-V exists and source exists, compare timestamps
            bool useSpvDirectly = false;
            if (spirvExists && srcExists)
            {
                try
                {
                    auto spvTime = std::filesystem::last_write_time(spirvPathStr);
                    auto srcTime = std::filesystem::last_write_time(srcPathStr);

                    useSpvDirectly = (spvTime >= srcTime);
                } catch (...)
                {
                    useSpvDirectly = spirvExists;
                }
            }
            else if (spirvExists)
            {
                useSpvDirectly = true;
            }
            else if (srcExists)
            {
                useSpvDirectly = false;
            }
            else
            {
                // neither exists
                continue;
            }

            // try load spirv directly
            if (useSpvDirectly)
            {
                auto spirvBytesOpt = ReadFileBinary(spirvPathStr);
                if (spirvBytesOpt)
                {
                    VkShaderModule mod = CreateShaderModuleFromBytes(device->m_vkDevice, *spirvBytesOpt);
                    if (mod != VK_NULL_HANDLE)
                    {
                        m_vkShaderModules[s] = mod;
                        continue;
                    }
                    else
                    {
                        std::cerr << "Failed to create shader module from existing SPV:" << spirvPathStr << "\n";
                    }
                }
                else
                {
                    std::cerr << "Failed to read existing SPV file:" << spirvPathStr << "\n";
                }
            }

            // try compile from source
            bool compiledSuccessfully = false;
            if (srcExists)
            {
                auto srcOpt = ReadFileText(srcPathStr);
                if (srcOpt)
                {
                    int kind = ShaderStageToShaderCKind((ShaderStage_t) s);
                    if (kind < 0)
                    {
                        std::cerr << "on runtime compiler support for stage" << StageVulkanName((ShaderStage_t) s) << "\n";
                        m_vkShaderModules[s] = VK_NULL_HANDLE;
                        continue;
                    }

                    shaderc::CompileOptions options;
                    // set includer
                    options.SetIncluder(std::make_unique<FileIncluder>(srcDir));
                    options.SetOptimizationLevel(shaderc_optimization_level_performance);

                    shaderc::SpvCompilationResult module =
                            compiler.CompileGlslToSpv(*srcOpt, static_cast<shaderc_shader_kind>(kind), srcPathStr.c_str(), options);
                    // shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(srcOpt->data(), // 传裸指针
                    //                                                                  srcOpt->size(), // 明确告诉长度（不包括额外 \0）
                    //                                                                  static_cast<shaderc_shader_kind>(kind), srcPathStr.c_str(),
                    //                                                                  "main", // entrypoint
                    //                                                                  options);
                    if (module.GetCompilationStatus() == shaderc_compilation_status_success)
                    {
                        // copy spirv
                        std::vector<uint32_t> spirv(module.cbegin(), module.cend());

                        if (writeSpvToDisk)
                        {
                            try
                            {
                                if (!std::filesystem::exists(spirvDir))
                                {
                                    std::filesystem::create_directories(spirvDir);
                                }
                                std::ofstream ofs(spirvPathStr, std::ios::binary);
                                if (ofs)
                                {
                                    ofs.write(reinterpret_cast<const char *>(spirv.data()), spirv.size() * sizeof(uint32_t));
                                    ofs.close();
                                    std::cout << "Write SPV to disk: " << spirvPathStr << " Successfully\n";
                                }
                                else
                                {
                                    std::cerr << "Failed to write SPV to disk: " << spirvPathStr << "\n";
                                }
                            } catch (const std::exception &e)
                            {
                                std::cerr << "Failed to write SPV to disk: " << e.what() << "\n";
                            }
                        }
                        VkShaderModule vkMod = CreateShaderModule(device->m_vkDevice, spirv.data(), static_cast<int>(spirv.size()));
                        if (vkMod != VK_NULL_HANDLE)
                        {
                            m_vkShaderModules[s] = vkMod;
                            compiledSuccessfully = true;
                            std::cout << "Compiled shader successfully: " << srcPathStr << "\n";
                            continue;
                        }
                        else
                        {
                            std::cerr << "Failed to create VkShaderModule from compiled shader: " << srcPathStr << "\n";
                        }
                    }
                    else
                    {
                        std::cerr << "Failed to compile shader: " << srcPathStr << ":" << module.GetErrorMessage() << "\n";
                        m_vkShaderModules[s] = VK_NULL_HANDLE;
                    }
                }
                else
                {
                    std::cerr << "Failed to read shader source file: " << srcPathStr << "\n";
                    m_vkShaderModules[s] = VK_NULL_HANDLE;
                }
            }

            // if spirv exists but we didn't pick it earlier (maybe it's older than src but no src), try Load spirv
            if (spirvExists && !compiledSuccessfully)
            {
                auto spirvBytesOpt = ReadFileBinary(spirvPathStr);
                if (spirvBytesOpt)
                {
                    VkShaderModule mod = CreateShaderModuleFromBytes(device->m_vkDevice, *spirvBytesOpt);
                    if (mod != VK_NULL_HANDLE)
                    {
                        m_vkShaderModules[s] = mod;
                        continue;
                    }
                }
            }

            // if reach here then nothing worked for this stage
            m_vkShaderModules[s] = VK_NULL_HANDLE;
        }
        return true;
    }
} // namespace ElecNeko
