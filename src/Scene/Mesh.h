#pragma once

#include <string>
#include <utility>
#include <vector>

#include "RHI/model.h"

namespace ElecNeko
{
    class Mesh
    {
    public:
        bool LoadFromFile(const std::string &filename);

        std::vector<vert_t> vertices;
        std::vector<uint32_t> indices;

        std::string name;
    };

    class MeshInstance
    {
    public:
        MeshInstance(std::string nam, const int meshID, const Mat4 &xform, const int matID) :
            transform(xform), name(std::move(nam)), materialId(matID), meshId(meshID)
        {}

    public:
        Mat4 transform;
        std::string name;

        int materialId;
        int meshId;
    };
} // namespace ElecNeko
