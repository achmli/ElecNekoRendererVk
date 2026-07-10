#include "CascadedShadow.h"

#include <cmath>
#include <cstdint>
#include <iostream>

#include "Math/LCP.h"

namespace ElecNeko
{
    struct CSMStaticMeshDrawPushConstant
    {
        uint32_t materialIndex;
        uint32_t instanceIndex;
    };

    static float mix(float a, float b, float t) { return (b - a) * t + a; }

    // world space view and invView of main camera, fov in radians, aspect(h/w), cascade range[cn,cf]
    std::array<Vec3, 8> GetFrustumCorners(const Mat4 &view, const Mat4 &invView, float fovY, float aspect, float cn, float cf)
    {
        std::array<Vec3, 8> corners;
        // for each depth (near and far) compute half width/height in view space
        auto makeCorners = [&](float z, int offset)
        {
            float h = tanf(fovY * 0.5f) * z;
            float w = h * aspect;

            Vec4 c0(-w, -h, -z, 1.0f);
            Vec4 c1(w, -h, -z, 1.0f);
            Vec4 c2(w, h, -z, 1.0f);
            Vec4 c3(-w, h, -z, 1.0f);
            Vec4 co0 = invView * c0;
            Vec4 co1 = invView * c1;
            Vec4 co2 = invView * c2;
            Vec4 co3 = invView * c3;
            corners[offset + 0] = Vec3(co0.x, co0.y, co0.z);
            corners[offset + 1] = Vec3(co1.x, co1.y, co1.z);
            corners[offset + 2] = Vec3(co2.x, co2.y, co2.z);
            corners[offset + 3] = Vec3(co3.x, co3.y, co3.z);
        };

        makeCorners(cn, 0);
        makeCorners(cf, 4);
        return corners;
    }

    Mat4 ComputeLightViewMatrix(const Vec3 &center, const Vec3 &sunDir)
    {
        Vec3 up = fabs(sunDir.Dot(Vec3(0, 1, 0))) > 0.9999f ? Vec3(0, 0, 1) : Vec3(0, 1, 0);
        Vec3 lightPos = center - sunDir * 1000.f;
        Mat4 viewMat;
        viewMat.LookAt(lightPos, center, up);
        return viewMat;
    }

    struct OrthoBox
    {
        float left, right, bottom, top, nearZ, farZ;
    };

    OrthoBox GetOrthoFromCorners(const std::array<Vec3, 8> &worldCorners, const Mat4 &lightView)
    {
        Vec3 minL(FLT_MAX), maxL(-FLT_MAX);
        for (auto &wc: worldCorners)
        {
            Vec4 lc4 = lightView * Vec4(wc.x, wc.y, wc.z, 1.0f);
            Vec3 lc = Vec3(lc4.x, lc4.y, lc4.z);
            minL = Vec3::ElecNekoMin(minL, lc);
            maxL = Vec3::ElecNekoMax(maxL, lc);
        }
        OrthoBox b;
        b.left = minL.x;
        b.right = maxL.x;
        b.bottom = minL.y;
        b.top = maxL.y;
        b.nearZ = minL.z - 10.f; // padding in world units
        b.farZ = maxL.z + 10.f;

        return b;
    }

    void StabilizeOrtho(OrthoBox &b, int shadowResolution)
    {
        float worldUnitsPerTexelX = (b.right - b.left) / static_cast<float>(shadowResolution);
        float worldUnitsPerTexelY = (b.top - b.bottom) / static_cast<float>(shadowResolution);

        // snap left/bottom to texel grid
        b.left = floor(b.left / worldUnitsPerTexelX) * worldUnitsPerTexelX;
        b.bottom = floor(b.bottom / worldUnitsPerTexelY) * worldUnitsPerTexelY;

        // recompute right/top to keep size integral multiples of texels
        b.right = b.left + worldUnitsPerTexelX * static_cast<float>(shadowResolution);
        b.top = b.bottom + worldUnitsPerTexelY * static_cast<float>(shadowResolution);
    }

    float ComputeShadowDistance(const std::array<Vec3, 8> &corners, Vec3 lightDir, float minDist, float maxDist, float margin, bool useProjExtent = true)
    {
        const float EPS = 1e-6;

        if (lightDir.GetLengthSqr() < EPS)
        {
            lightDir = Vec3(0.f, 1.f, 0.f);
        }
        else
        {
            lightDir.Normalize();
        }

        // compute center of frustum
        Vec3 center(0.f);
        for (int i = 0; i < 8; ++i)
        {
            center += corners[i];
        }
        center /= 8.f;

        // compute radius of bounding sphere
        float radius = 0.f;
        for (int i = 0; i < 8; ++i)
        {
            float dist = (corners[i] - center).GetMagnitude();
            if (dist > radius)
            {
                radius = dist;
            }
        }

        float base = radius;

        if (useProjExtent)
        {
            float minP = FLT_MAX;
            float maxP = -FLT_MAX;
            for (int i = 0; i < 8; i++)
            {
                float p = (corners[i] - center).Dot(lightDir);
                if (p < minP)
                {
                    minP = p;
                }
                if (p > maxP)
                {
                    maxP = p;
                }
            }
            float halfExtent = (maxP - minP) * 0.5f;
            base = std::min(radius, halfExtent);
        }

        float distance = base + margin;

        distance = std::clamp(distance, minDist, maxDist);

        return distance;
    }

    void DebugLightOrientation(const Mat4 &lightView, const Vec3 &newCenterWorld, const Vec3 &sunDir)
    {
        auto toLS = [&](const Vec3 &p) -> Vec3
        {
            Vec4 t = lightView * Vec4(p.x, p.y, p.z, 1.0f);
            return Vec3(t.x, t.y, t.z);
        };

        Vec3 centerLS = toLS(newCenterWorld);
        Vec3 aboveWorld = newCenterWorld + Vec3(0.0f, 1.0f, 0.0f) * 5.0f; // world-space 5 units above center
        Vec3 belowWorld = newCenterWorld - Vec3(0.0f, 1.0f, 0.0f) * 5.0f;

        Vec3 aboveLS = toLS(aboveWorld);
        Vec3 belowLS = toLS(belowWorld);

        printf("centerLS z = %f, aboveLS z = %f, belowLS z = %f\n", centerLS.z, aboveLS.z, belowLS.z);
        printf("sunDir = (%f,%f,%f)\n", sunDir.x, sunDir.y, sunDir.z);

        // sign test for light direction relative to world up
        float dotUp = sunDir.Dot(Vec3(0, 1, 0));
        printf("dot(sunDir, worldUp) = %f\n", dotUp);

        // light position relative to center
        // compute eye from lookAt via inverse if you have lightView
        Mat4 inv = lightView.Inverse(); // attention your Inverse right or not
        Vec4 eye4 = inv * Vec4(0, 0, 0, 1); // light-space origin maps to world eye pos
        printf("lightEyeWorld ~= (%f,%f,%f)\n", eye4.x, eye4.y, eye4.z);
    }

    Mat4 ComputeLightViewMatrixFromFrumstum(OrthoBox &box, const std::array<Vec3, 8> &frustumCornersWorld, const Vec3 &sunDir, bool stabilized,
                                            float shadowDistance, float mergeN, float mergeF)
    {
        Vec3 center(0.0f);
        for (int i = 0; i < 8; ++i)
        {
            center += frustumCornersWorld[i];
        }
        center /= 8;

        Vec3 up = fabs(sunDir.Dot(Vec3(0.f, 1.f, 0.f))) > 0.999f ? Vec3(0.f, 0.f, 1.f) : Vec3(0.f, 1.f, 0.f);
        // Vec3 up = Vec3(0.f, 1.f, 0.f);

        Vec3 lightDir = sunDir;
        lightDir.Normalize();
        // lightDir = -lightDir;
        Vec3 lightPos = center + lightDir * shadowDistance;
        Mat4 lightView;
        lightView.LookAt(lightPos, center, up);

        // transform into light space, compute AABB
        Vec3 minLS(FLT_MAX);
        Vec3 maxLS(-FLT_MAX);
        for (int i = 0; i < 8; ++i)
        {
            Vec4 pLS = lightView * Vec4(frustumCornersWorld[i].x, frustumCornersWorld[i].y, frustumCornersWorld[i].z, 1.0f);
            Vec3 p = Vec3(pLS.x, pLS.y, pLS.z);
            minLS = Vec3::ElecNekoMin(minLS, p);
            maxLS = Vec3::ElecNekoMax(maxLS, p);
        }

        box.left = minLS.x - mergeF * shadowDistance;
        box.right = maxLS.x + mergeF * shadowDistance;
        box.bottom = minLS.y - mergeF * shadowDistance;
        box.top = maxLS.y + mergeF * shadowDistance;
        box.nearZ = minLS.z - mergeN * shadowDistance;
        box.farZ = maxLS.z + mergeF * shadowDistance;

        if (stabilized)
        {
            float texelSizeWorldX = (box.right - box.left) / 4096.f;
            float texelSizeWorldY = (box.top - box.bottom) / 4096.f;

            box.left = floor(box.left / texelSizeWorldX) * texelSizeWorldX;
            box.bottom = floor(box.bottom / texelSizeWorldY) * texelSizeWorldY;

            box.right = box.left + texelSizeWorldX * 4096.f;
            box.top = box.bottom + texelSizeWorldY * 4096.f;
        }

        // center in light space
        float centerX = (box.left + box.right) * 0.5f;
        float centerY = (box.bottom + box.top) * 0.5f;
        float centerZ = (box.nearZ + box.farZ) * 0.5f;

        Mat4 invLightView = lightView.Inverse();
        Vec4 centerWorld4 = invLightView * Vec4(centerX, centerY, centerZ, 1.0);
        Vec3 newCenterWorld(centerWorld4.x, centerWorld4.y, centerWorld4.z);

        Vec3 finalLightPos = newCenterWorld + lightDir * shadowDistance;
        Mat4 finalLightView;
        finalLightView.LookAt(finalLightPos, newCenterWorld, up);

        // DebugLightOrientation(finalLightView, newCenterWorld, sunDir);

        return finalLightView;
    }

    bool CascadeShadow::Initialize(DeviceContext *device)
    {
        m_shadowMaps.resize(numCascade);
        for (int i = 0; i < numCascade; i++)
        {
            FrameBuffer::CreateParms_t frameBufferParms{};
            frameBufferParms.width = 4096;
            frameBufferParms.height = 4096;
            frameBufferParms.hasColor = false;
            frameBufferParms.hasDepth = true;
            if (!m_shadowMaps[i].Create(device, frameBufferParms))
            {
                std::cerr << "Failed to Create cascaded shadow map!\n";
                return false;
            }
        }

        // {
        //     if (!m_shader.Load(device, "csmGen"))
        //     {
        //         std::cerr << "Failed to Load csmGen Shader\n";
        //         return false;
        //     }

        //     ElecNekoDescriptors::CreateParms_t descriptorParms{};
        //     memset(&descriptorParms, 0, sizeof(descriptorParms));
        //     descriptorParms.numUniformsVertex = 2;
        //     descriptorParms.numStorageVertex = 1;
        //     descriptorParms.numUniformsFragment = 0;
        //     descriptorParms.numStorageFragment = 0;
        //     descriptorParms.numImageSamplers = 0;
        //     if (!m_descriptors.Create(device, descriptorParms))
        //     {
        //         std::cerr << "Failed to Load csmGen Descriptors\n";
        //         return false;
        //     }

        //     m_pipelines.resize(numCascade);
        //     for (int i = 0; i < numCascade; ++i)
        //     {
        //         ElecNekoPipeline::CreateParms_t pipelineParms{};
        //         pipelineParms.framebuffer = &m_shadowMaps[i];
        //         pipelineParms.descriptors = &m_descriptors;
        //         pipelineParms.shader = &m_shader;
        //         pipelineParms.width = 4096;
        //         pipelineParms.height = 4096;
        //         pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_BACK;
        //         pipelineParms.depthTest = true;
        //         pipelineParms.depthWrite = true;
        //         if (!m_pipelines[i].Create(device, pipelineParms, ElecNekoPipeline::USAGE_MESH))
        //         {
        //             std::cerr << "Failed to Load csmGen pipeline\n";
        //             return false;
        //         }
        //     }
        // }

        m_viewMatrixBuffers.Allocate(device, nullptr, sizeof(Mat4) * numCascade, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        m_projMatrixBuffers.Allocate(device, nullptr, sizeof(Mat4) * numCascade, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        m_uniformBuffer.Allocate(device, nullptr, sizeof(ShadowUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        {
            if (!m_staticMeshShader.Load(device, "csmStaticMesh"))
            {
                std::cerr << "Failed to Load csmStaticMesh Shader\n";
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms{};
            memset(&descriptorParms, 0, sizeof(descriptorParms));

            // binding 0: view matrix uniform
            // binding 1: proj matrix uniform
            // binding 2: RenderScene instance storage buffer
            descriptorParms.numUniformsVertex = 2;
            descriptorParms.numStorageVertex = 1;
            descriptorParms.numUniformsFragment = 0;
            descriptorParms.numStorageFragment = 0;
            descriptorParms.numImageSamplers = 0;

            if (!m_staticMeshDescriptors.Create(device, descriptorParms))
            {
                std::cerr << "Failed to Load csmStaticMesh Descriptors\n";
                return false;
            }

            m_staticMeshPipelines.resize(numCascade);

            for (int i = 0; i < numCascade; ++i)
            {
                ElecNekoPipeline::CreateParms_t pipelineParms{};
                pipelineParms.framebuffer = &m_shadowMaps[i];
                pipelineParms.descriptors = &m_staticMeshDescriptors;
                pipelineParms.shader = &m_staticMeshShader;
                pipelineParms.width = 4096;
                pipelineParms.height = 4096;
                pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_BACK;
                pipelineParms.depthTest = true;
                pipelineParms.depthWrite = true;
                pipelineParms.pushConstantSize = sizeof(CSMStaticMeshDrawPushConstant);
                pipelineParms.pushConstantShaderStages = VK_SHADER_STAGE_VERTEX_BIT;

                if (!m_staticMeshPipelines[i].Create(device, pipelineParms, ElecNekoPipeline::USAGE_STATIC_MESH))
                {
                    std::cerr << "Failed to Load csmStaticMesh pipeline\n";
                    return false;
                }
            }
        }

        {
            if (!m_staticMeshMaskShader.Load(device, "csmStaticMeshMasked"))
            {
                std::cerr << "Failed to Load csmStaticMeshMasked Shader\n";
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms{};
            memset(&descriptorParms, 0, sizeof(descriptorParms));

            // binding 0: view matrix uniform
            // binding 1: proj matrix uniform
            // binding 2: RenderScene instance buffer
            // binding 3: RenderScene material buffer
            // binding 4: RenderScene texture array
            descriptorParms.numUniformsVertex = 2;
            descriptorParms.numStorageVertex = 1;
            descriptorParms.numUniformsFragment = 0;
            descriptorParms.numStorageFragment = 1;
            descriptorParms.numImageSamplers = 1;

            if (!m_staticMeshMaskDescriptors.Create(device, descriptorParms))
            {
                std::cerr << "Failed to Load csmStaticMeshMasked Descriptors\n";
                return false;
            }

            m_staticMeshMaskPipelines.resize(numCascade);

            for (int i = 0; i < numCascade; ++i)
            {
                ElecNekoPipeline::CreateParms_t pipelineParms{};
                pipelineParms.framebuffer = &m_shadowMaps[i];
                pipelineParms.descriptors = &m_staticMeshMaskDescriptors;
                pipelineParms.shader = &m_staticMeshMaskShader;
                pipelineParms.width = 4096;
                pipelineParms.height = 4096;
                pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_BACK;
                pipelineParms.depthTest = true;
                pipelineParms.depthWrite = true;
                pipelineParms.pushConstantSize = sizeof(CSMStaticMeshDrawPushConstant);
                pipelineParms.pushConstantShaderStages = VK_SHADER_STAGE_VERTEX_BIT;

                if (!m_staticMeshMaskPipelines[i].Create(device, pipelineParms, ElecNekoPipeline::USAGE_STATIC_MESH))
                {
                    std::cerr << "Failed to Load csmStaticMeshMasked pipeline\n";
                    return false;
                }
            }
        }

        // {
        //     if (!m_maskShader.Load(device, "csmMaskGen"))
        //     {
        //         std::cerr << "Failed to Load csmMaskGen Shader\n";
        //         return false;
        //     }

        //     ElecNekoDescriptors::CreateParms_t descriptorParms{};
        //     memset(&descriptorParms, 0, sizeof(descriptorParms));
        //     descriptorParms.numUniformsVertex = 2;
        //     descriptorParms.numStorageVertex = 1;
        //     descriptorParms.numUniformsFragment = 0;
        //     descriptorParms.numStorageFragment = 1;
        //     descriptorParms.numImageSamplers = 1;
        //     if (!m_maskDescriptors.Create(device, descriptorParms))
        //     {
        //         std::cerr << "Failed to Load csmMaskGen Descriptors\n";
        //         return false;
        //     }

        //     m_maskPipelines.resize(numCascade);
        //     for (int i = 0; i < numCascade; ++i)
        //     {
        //         ElecNekoPipeline::CreateParms_t pipelineParms{};
        //         pipelineParms.framebuffer = &m_shadowMaps[i];
        //         pipelineParms.descriptors = &m_maskDescriptors;
        //         pipelineParms.shader = &m_maskShader;
        //         pipelineParms.width = 4096;
        //         pipelineParms.height = 4096;
        //         pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_BACK;
        //         pipelineParms.depthTest = true;
        //         pipelineParms.depthWrite = true;
        //         if (!m_maskPipelines[i].Create(device, pipelineParms, ElecNekoPipeline::USAGE_MESH))
        //         {
        //             std::cerr << "Failed to Load csmMaskGen pipeline\n";
        //             return false;
        //         }
        //     }
        // }

        return true;
    }

    void CascadeShadow::ComputeCascadeSplits()
    {
        m_splits.resize(numCascade + 1);
        m_splits[0] = minDistance;
        for (int i = 1; i <= numCascade; ++i)
        {
            float id = static_cast<float>(i) / static_cast<float>(numCascade);
            // logarithmic split
            float logSplit = minDistance * powf(maxDistance / minDistance, id);
            // uniform split
            float uniSplit = minDistance + (maxDistance - minDistance) * id;

            m_splits[i] = mix(uniSplit, logSplit, lambda);
        }
    }

    void CascadeShadow::Update(const Vec3 &lightDir, float aspect)
    {
        m_views.clear();
        m_projections.clear();
        m_splits.clear();

        ComputeCascadeSplits();

        std::vector<std::array<Vec3, 8>> corners;
        for (int i = 0; i < numCascade; i++)
        {
            std::array<Vec3, 8> corner;
            corner = GetFrustumCorners(view, invView, fov, aspect, m_splits[i], m_splits[i + 1]);
            corners.push_back(corner);
        }

        m_views.reserve(numCascade);
        m_projections.reserve(numCascade);
        for (const auto &corner: corners)
        {
            float d = ComputeShadowDistance(corner, lightDir, 2.f, 1000.f, 1.5f, true);
            OrthoBox box;
            Mat4 view;
            view = ComputeLightViewMatrixFromFrumstum(box, corner, lightDir, stabilizeTexels, d, mergeN, mergeF);
            m_views.push_back(view.Transpose());
            Mat4 proj;
            proj.OrthoVulkan(box.left, box.right, box.bottom, box.top, box.nearZ, box.farZ);
            m_projections.push_back(proj.Transpose());
        }
    }

    void CascadeShadow::UpdateDistance(float cn, float cf)
    {
        minDistance = cn;
        maxDistance = cf;
    }

    void CascadeShadow::UpdateMainView(const Mat4 &camView, const Mat4 &camInvView, float fovY)
    {
        view = camView;
        invView = camInvView;
        fov = Radians(fovY);
    }


    bool CascadeShadow::MakeUBO(DeviceContext *device)
    {
        // VkDeviceSize bufferSize = numCascade * sizeof(Mat4);
        // if (!m_viewMatrixBuffers.Allocate(device, m_views.data(), bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
        // {
        //     std::cerr << "Failed to allocate view matrix buffer!";
        //     assert(0);
        //     return false;
        // }
        {
            unsigned char *mappedData = (unsigned char *) m_viewMatrixBuffers.MapBuffer(device);
            memcpy(mappedData, m_views.data(), sizeof(Mat4) * numCascade);
            m_viewMatrixBuffers.UnmapBuffer(device);
        }

        // if (!m_projMatrixBuffers.Allocate(device, m_projections.data(), bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
        // {
        //     std::cerr << "Failed to allocate projection matrix buffer!";
        //     assert(0);
        //     return false;
        // }
        {
            unsigned char *mappedData = (unsigned char *) m_projMatrixBuffers.MapBuffer(device);
            memcpy(mappedData, m_projections.data(), sizeof(Mat4) * numCascade);
            m_projMatrixBuffers.UnmapBuffer(device);
        }

        ShadowUniforms uniforms;
        uniforms.numCascades = numCascade;
        uniforms.texelSize = 1.f / 4096.f;
        uniforms.splits[0] = m_splits[0];
        uniforms.splits[1] = m_splits[1];
        uniforms.splits[2] = numCascade > 1 ? m_splits[2] : 0.f;
        uniforms.splits[3] = numCascade > 2 ? m_splits[3] : 0.f;
        uniforms.splits[4] = numCascade > 3 ? m_splits[4] : 0.f;
        if (visualizeCascade)
        {
            uniforms.visualize = 1;
        }
        else
        {
            uniforms.visualize = 0;
        }
        // bufferSize = sizeof(ShadowUniforms);
        // if (!m_uniformBuffer.Allocate(device, &uniforms, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
        // {
        //     std::cerr << "Failed to allocate shadow parameters buffer!";
        //     assert(0);
        //     return false;
        // }
        {
            unsigned char *mappedData = (unsigned char *) m_uniformBuffer.MapBuffer(device);
            memcpy(mappedData, &uniforms, sizeof(ShadowUniforms));
            m_uniformBuffer.UnmapBuffer(device);
        }

        return true;
    }

    void CascadeShadow::Cleanup(DeviceContext *device)
    {
        for (auto &map: m_shadowMaps)
        {
            map.Cleanup(device);
        }
        m_shadowMaps.clear();
        m_views.clear();
        m_projections.clear();
        m_splits.clear();

        // for (int i = 0; i < numCascade; ++i)
        // {
        //     m_pipelines[i].Cleanup(device);
        //     m_maskPipelines[i].Cleanup(device);
        // }
        // m_pipelines.clear();
        // m_maskPipelines.clear();

        // m_shader.Cleanup(device);
        // m_maskShader.Cleanup(device);

        // m_descriptors.Cleanup(device);
        // m_maskDescriptors.Cleanup(device);
        for (int i = 0; i < numCascade; ++i)
        {
            // m_pipelines[i].Cleanup(device);
            m_staticMeshPipelines[i].Cleanup(device);
            m_staticMeshMaskPipelines[i].Cleanup(device);
            // m_maskPipelines[i].Cleanup(device);
        }

        // m_pipelines.clear();
        m_staticMeshPipelines.clear();
        m_staticMeshMaskPipelines.clear();
        // m_maskPipelines.clear();

        // m_shader.Cleanup(device);
        m_staticMeshShader.Cleanup(device);
        m_staticMeshMaskShader.Cleanup(device);
        // m_maskShader.Cleanup(device);

        // m_descriptors.Cleanup(device);
        m_staticMeshDescriptors.Cleanup(device);
        m_staticMeshMaskDescriptors.Cleanup(device);
        // m_maskDescriptors.Cleanup(device);

        m_viewMatrixBuffers.Cleanup(device);
        m_projMatrixBuffers.Cleanup(device);
        m_uniformBuffer.Cleanup(device);
    }

} // namespace ElecNeko
