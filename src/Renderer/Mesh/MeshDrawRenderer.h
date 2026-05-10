// src/Renderer/Mesh/MeshDrawRenderer.h
#pragma once

#include "Renderer/Mesh/MeshDrawCommand.h"

#include <cstdint>
#include <vector>

class RHICommandList;

namespace ElecNeko
{
    class RenderScene;
    class ElecNekoPipeline;

    struct MeshDrawPushConstant
    {
        uint32_t materialIndex = 0;
        uint32_t instanceIndex = 0;
    };

    class MeshDrawRenderer
    {
    public:
        static void DrawDrawList(RHICommandList &cmd, ElecNekoPipeline &pipeline, const std::vector<MeshDrawCommand> &draws);

        static void DrawOpaque(RHICommandList &cmd, ElecNekoPipeline &pipeline, const RenderScene &scene);

        static void DrawMasked(RHICommandList &cmd, ElecNekoPipeline &pipeline, const RenderScene &scene);

        static void DrawTransparent(RHICommandList &cmd, ElecNekoPipeline &pipeline, const RenderScene &scene);

        static void DrawShadow(RHICommandList &cmd, ElecNekoPipeline &pipeline, const RenderScene &scene);
    };
} // namespace ElecNeko
