#pragma once

#include "Renderer/Mesh/MeshDrawCommand.h"

#include <vector>

namespace ElecNeko
{
    class MeshDrawList
    {
    public:
        std::vector<MeshDrawCommand> opaque;
        std::vector<MeshDrawCommand> masked;
        std::vector<MeshDrawCommand> transparent;
        std::vector<MeshDrawCommand> shadow;
        std::vector<MeshDrawCommand> depthOnly;

        void Clear()
        {
            opaque.clear();
            masked.clear();
            transparent.clear();
            shadow.clear();
            depthOnly.clear();
        }
    };
} // namespace ElecNeko
