//
// Created by ElecNekoSurface on 25-3-17.
//

#include "Mesh.h"

#include <algorithm>
#include <filesystem>

#include "../tinyobjloader/tiny_obj_loader.h"

namespace ElecNeko
{
    unsigned char FloatToBytes(const float f)
    {
        int i = static_cast<int>(f * 127 + 128);
        return static_cast<unsigned char>(i);
    }

    bool Mesh::LoadFromFile(const std::string &filename)
    {
        name = filename;
        std::filesystem::path p(name);

        auto matPath = p.parent_path();
        tinyobj::ObjReaderConfig readerConfig;
        readerConfig.mtl_search_path = matPath.string();

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(filename, readerConfig))
        {
            if (!reader.Error().empty())
            {
                printf("TinyObjLoader: %s \n", reader.Error().c_str());
            }

            return false;
        }

        if (!reader.Warning().empty())
        {
            printf("TinyObjLoader: %s \n", reader.Warning().c_str());
        }

        auto &attrib = reader.GetAttrib();
        auto &shapes = reader.GetShapes();
        auto &materials = reader.GetMaterials();

        size_t totalIndices = 0;
        for (const auto &shape: shapes)
        {
            totalIndices += shape.mesh.indices.size();
        }
        vertices.reserve(totalIndices);
        indices.reserve(totalIndices);

        for (const auto &shape: shapes)
        {
            size_t indexOffset = 0;

            for (const auto &face: shape.mesh.num_face_vertices)
            {
                // loop over vertices in the face
                for (size_t v = 0; v < 3; v++)
                {
                    // access to vertex
                    tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];

                    vert_t vertex = {};
                    vertex.xyz[0] = attrib.vertices[3 * idx.vertex_index + 0];
                    vertex.xyz[1] = attrib.vertices[3 * idx.vertex_index + 1];
                    vertex.xyz[2] = attrib.vertices[3 * idx.vertex_index + 2];

                    if (idx.normal_index >= 0 && !attrib.normals.empty())
                    {
                        vertex.norm[0] = FloatToBytes(attrib.normals[3 * idx.normal_index + 0]);
                        vertex.norm[1] = FloatToBytes(attrib.normals[3 * idx.normal_index + 1]);
                        vertex.norm[2] = FloatToBytes(attrib.normals[3 * idx.normal_index + 2]);
                    }
                    else
                    {
                        vertex.norm[0] = 128;
                        vertex.norm[1] = 255;
                        vertex.norm[2] = 128;
                    }

                    if (idx.texcoord_index >= 0 && !attrib.texcoords.empty())
                    {
                        vertex.st[0] = attrib.texcoords[2 * idx.texcoord_index + 0];
                        vertex.st[1] = 1.f - attrib.texcoords[2 * idx.texcoord_index + 1];
                    }
                    else
                    {
                        if (v == 0)
                            vertex.st[0] = vertex.st[1] = 0;
                        else if (v == 1)
                            vertex.st[0] = 0, vertex.st[1] = 1;
                        else
                            vertex.st[0] = vertex.st[1] = 1;
                    }

                    indices.emplace_back(static_cast<uint32_t>(vertices.size()));
                    vertices.emplace_back(vertex);
                }

                indexOffset += 3;
            }
        }

        return true;
    }
} // namespace ElecNeko
