// src/Renderer/Mesh/AssimpStaticMeshLoader.h
#pragma once

#include "Renderer/Mesh/StaticMesh.h"

#include <string>

struct aiMesh;

namespace ElecNeko
{
    class AssimpStaticMeshLoader
    {
    public:
        static StaticMeshAsset BuildStaticMeshAssetFromAiMesh(const aiMesh *mesh, const std::string &name, uint32_t materialIndex = 0);
    };
} // namespace ElecNeko
