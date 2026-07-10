// src/Renderer/Mesh/StaticMeshAssetBuilder.h
#pragma once

#include "Renderer/Mesh/StaticMesh.h"

namespace ElecNeko
{
    class ElecNekoMesh;

    class StaticMeshAssetBuilder
    {
    public:
        static StaticMeshAsset BuildFromLegacyMesh(const ElecNekoMesh &legacyMesh);
    };
} // namespace ElecNeko
