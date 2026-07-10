// src/Renderer/Import/AssimpGeometryImporter.h
#pragma once

#include "Renderer/Mesh/StaticMesh.h"

#include <filesystem>
#include <string>

namespace ElecNeko
{
    struct GeometryImportOptions
    {
        bool triangulate = true;
        bool generateNormals = false;
        bool generateTangents = false;
        bool flipUVs = false;

        bool mergeMeshes = true;
        bool deduplicateVertices = false;
    };

    struct GeometryImportResult
    {
        bool success = false;
        std::string errorMessage;

        StaticMeshAsset mesh;
    };

    class AssimpGeometryImporter
    {
    public:
        static GeometryImportResult ImportGeometryFile(const std::filesystem::path &filePath, const GeometryImportOptions &options = GeometryImportOptions{});
    };
} // namespace ElecNeko
