// src/Renderer/Mesh/MeshDrawRenderer.cpp
#include "Renderer/Mesh/MeshDrawRenderer.h"

#include "Renderer/Scene/RenderScene.h"

#include "RHI/Pipeline.h"
#include "RHI/RHICommandList.h"

#include <cassert>

namespace ElecNeko
{
    void MeshDrawRenderer::DrawDrawList(RHICommandList &cmd, ElecNekoPipeline &pipeline, const std::vector<MeshDrawCommand> &draws)
    {
        assert(cmd.IsValid());

        VkCommandBuffer vkCmd = cmd.GetNativeCommandBuffer();

        for (const MeshDrawCommand &draw: draws)
        {
            if (!draw.vertexBuffer.IsValid() || !draw.indexBuffer.IsValid())
            {
                continue;
            }

            if (draw.indexCount == 0)
            {
                continue;
            }

            MeshDrawPushConstant push{};
            push.materialIndex = draw.materialIndex;
            push.instanceIndex = draw.instanceIndex;

            pipeline.PushConstants(vkCmd, &push, sizeof(MeshDrawPushConstant));

            cmd.SetVertexBuffer(draw.vertexBuffer, 0);
            cmd.SetIndexBuffer(draw.indexBuffer, RHIIndexFormat::UInt32);

            cmd.DrawIndexed(draw.indexCount, 1, draw.firstIndex, draw.vertexOffset, 0);
        }
    }

    void MeshDrawRenderer::DrawOpaque(RHICommandList &cmd, ElecNekoPipeline &pipeline, const RenderScene &scene)
    {
        DrawDrawList(cmd, pipeline, scene.drawList.opaque);
    }

    void MeshDrawRenderer::DrawMasked(RHICommandList &cmd, ElecNekoPipeline &pipeline, const RenderScene &scene)
    {
        DrawDrawList(cmd, pipeline, scene.drawList.masked);
    }

    void MeshDrawRenderer::DrawTransparent(RHICommandList &cmd, ElecNekoPipeline &pipeline, const RenderScene &scene)
    {
        DrawDrawList(cmd, pipeline, scene.drawList.transparent);
    }

    void MeshDrawRenderer::DrawShadow(RHICommandList &cmd, ElecNekoPipeline &pipeline, const RenderScene &scene)
    {
        DrawDrawList(cmd, pipeline, scene.drawList.shadow);
    }
} // namespace ElecNeko
