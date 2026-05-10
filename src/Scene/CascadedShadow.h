#pragma once

#include "Camera.h"
#include "RHI/ElecNekoShader.h"
#include "RHI/FrameBuffer.h"
#include "RHI/Pipeline.h"

namespace ElecNeko
{
    struct ShadowUniforms
    {
        float splits[5];
        float texelSize;
        uint32_t numCascades;
        int32_t visualize;
        float paddings[8] = {0.f};
    };
    class CascadeShadow
    {
    public:
        CascadeShadow() : minDistance(0), maxDistance(0), numCascade(4), lambda(0.0f) {};
        ~CascadeShadow() = default;

        CascadeShadow(float near, float far, uint32_t numImages = 4, float p = 0.f) : minDistance(near), maxDistance(far), numCascade(numImages), lambda(p) {}

        bool Initialize(DeviceContext *device);
        void Cleanup(DeviceContext *device);
        bool MakeUBO(DeviceContext *device);

        void ComputeCascadeSplits();

        void Update(const Vec3 &lightDir, float aspect);
        void UpdateDistance(float cn, float cf);
        void UpdateMainView(const Mat4 &camView, const Mat4 &camInvView, float fovY);

    public:
        std::vector<FrameBuffer> m_shadowMaps;
        std::vector<ElecNekoPipeline> m_pipelines;
        ElecNekoShader m_shader;
        ElecNekoDescriptors m_descriptors;

        std::vector<ElecNekoPipeline> m_staticMeshPipelines;
        ElecNekoShader m_staticMeshShader;
        ElecNekoDescriptors m_staticMeshDescriptors;

        std::vector<ElecNekoPipeline> m_staticMeshMaskPipelines;
        ElecNekoShader m_staticMeshMaskShader;
        ElecNekoDescriptors m_staticMeshMaskDescriptors;

        std::vector<ElecNekoPipeline> m_maskPipelines;
        ElecNekoShader m_maskShader;
        ElecNekoDescriptors m_maskDescriptors;

        std::vector<Mat4> m_views;
        std::vector<Mat4> m_projections;

        Mat4 view;
        Mat4 invView;
        float fov;

        float minDistance;
        float maxDistance;

        // 4, but todo: support 1, 2, 3...
        uint32_t numCascade;
        std::vector<float> m_splits;

        float lambda; // uniform split or logarithmic split, mix
        float mergeF = 2.5;
        float mergeN = 1.0;
        bool stabilizeTexels = false;
        bool visualizeCascade = false;

        Buffer m_viewMatrixBuffers;
        Buffer m_projMatrixBuffers;
        Buffer m_uniformBuffer;
    };
} // namespace ElecNeko
