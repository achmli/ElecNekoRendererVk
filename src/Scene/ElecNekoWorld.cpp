#include "ElecNekoWorld.h"

#include <fstream>

#include <assimp/pbrmaterial.h>

namespace ElecNeko
{
    bool ElecNekoModel::MakeVBO(DeviceContext *device)
    {
        if (m_vertices.empty())
        {
            return true;
        }

        int vbSize = static_cast<int>(sizeof(VVertex) * m_vertices.size());
        if (!m_vertexBuffer.Allocate(device, m_vertices.data(), vbSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
        {
            printf("Failed to create vertex buffer for model %s\n", name.c_str());
            return false;
        }

        int ibSize = static_cast<int>(sizeof(uint32_t) * m_indices.size());
        if (!m_indexBuffer.Allocate(device, m_indices.data(), ibSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
        {
            printf("Failed to create index buffer for model %s\n", name.c_str());
            return false;
        }

        return true;
    }

    void ElecNekoModel::Cleanup(DeviceContext *device)
    {
        m_vertexBuffer.Cleanup(device);
        m_indexBuffer.Cleanup(device);
        m_vertices.clear();
        m_indices.clear();
        subMeshes.clear();
    }

    Mat4 ElecNekoWorld::AiToMat4(const aiMatrix4x4 &aiMat)
    {
        Mat4 mat;
        mat.rows[0] = Vec4(aiMat.a1, aiMat.a2, aiMat.a3, aiMat.a4);
        mat.rows[1] = Vec4(aiMat.b1, aiMat.b2, aiMat.b3, aiMat.b4);
        mat.rows[2] = Vec4(aiMat.c1, aiMat.c2, aiMat.c3, aiMat.c4);
        mat.rows[3] = Vec4(aiMat.d1, aiMat.d2, aiMat.d3, aiMat.d4);
        return mat;
    }

    aiMatrix4x4 ElecNekoWorld::Mat4ToAiMatrix4x4(const Mat4 &mat)
    {
        aiMatrix4x4 aiMat;
        aiMat.a1 = mat.rows[0].x;
        aiMat.a2 = mat.rows[0].y;
        aiMat.a3 = mat.rows[0].z;
        aiMat.a4 = mat.rows[0].w;

        aiMat.b1 = mat.rows[1].x;
        aiMat.b2 = mat.rows[1].y;
        aiMat.b3 = mat.rows[1].z;
        aiMat.b4 = mat.rows[1].w;

        aiMat.c1 = mat.rows[2].x;
        aiMat.c2 = mat.rows[2].y;
        aiMat.c3 = mat.rows[2].z;
        aiMat.c4 = mat.rows[2].w;

        aiMat.d1 = mat.rows[3].x;
        aiMat.d2 = mat.rows[3].y;
        aiMat.d3 = mat.rows[3].z;
        aiMat.d4 = mat.rows[3].w;

        return aiMat;
    }

    std::string ElecNekoWorld::Trim(std::string_view v)
    {
        size_t b = 0, e = v.size();
        while (b < e && std::isspace((unsigned char) v[b]))
        {
            ++b;
        }
        while (e > b && std::isspace((unsigned char) v[e - 1]))
        {
            --e;
        }
        return std::string(v.substr(b, e - b));
    }

    std::vector<std::string> ElecNekoWorld::ReadBlock(std::ifstream &ifs, const std::string &firstLine)
    {
        std::vector<std::string> out;

        bool inBlock = (firstLine.find('{') != std::string::npos);
        if (!inBlock)
        {
            std::string line;
            while (std::getline(ifs, line))
            {
                auto t = Trim(line);
                if (t == "{" || (!t.empty() && t.front() == '{'))
                {
                    inBlock = true;
                    break;
                }
                if (t.empty())
                {
                    continue;
                }
            }
            if (!inBlock)
            {
                return out;
            }
        }

        std::string line;
        while (std::getline(ifs, line))
        {
            std::string t = Trim(line);
            if (t.empty())
            {
                continue;
            }
            if (t.find('}') != std::string::npos)
            {
                break;
            }
            out.push_back(t);
        }

        return out;
    }

    std::optional<std::array<float, 3>> ElecNekoWorld::ParseVec3(const std::string &line)
    {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key))
            return std::nullopt;

        std::array<float, 3> v{0, 0, 0};
        if (iss >> v[0] >> v[1] >> v[2])
            return v;

        return std::nullopt;
    }

    std::optional<std::pair<int, int>> ElecNekoWorld::ParseTwoInts(const std::string &line)
    {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key))
        {
            return std::nullopt;
        }
        int a, b;
        if (iss >> a >> b)
        {
            return std::pair<int, int>{a, b};
        }
        return std::nullopt;
    }

    std::string ElecNekoWorld::ParseSingleToken(const std::string &line)
    {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key))
        {
            return "";
        }
        /*std::string token;
        iss >> token;
        return token;*/
        std::string rest;
        std::getline(iss, rest);
        // trim rest
        size_t b = 0, e = rest.size();
        while (b < e && std::isspace((unsigned char) rest[b]))
        {
            ++b;
        }
        while (e > b && std::isspace((unsigned char) rest[e - 1]))
        {
            --e;
        }
        return std::string(rest.substr(b, e - b));
    }

    float ElecNekoWorld::ParseSingleFloat(const std::string &line, float fallback)
    {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key))
        {
            return fallback;
        }
        float v;
        if (iss >> v)
        {
            return v;
        }
        return fallback;
    }

    std::string ElecNekoWorld::NormalizePathForKey(const std::string &path)
    {
        try
        {
            return std::filesystem::weakly_canonical(std::filesystem::path(path)).string();
        } catch (...)
        {
            return path;
        }
    }

    std::string ElecNekoWorld::SaveEmbeddedTextureToTempFile(const aiTexture *texture, const std::filesystem::path &modelFilePath, int embIndex)
    {
        if (!texture)
        {
            return "";
        }

        if (texture->mHeight == 0 && texture->pcData)
        {
            std::string fmt = "";
            if (texture->achFormatHint && texture->achFormatHint[0] != '\0')
            {
                fmt = std::string(texture->achFormatHint);
                auto pos = fmt.find('\0');
                if (pos != std::string::npos)
                {
                    fmt.resize(pos);
                }
            }
            if (fmt.empty())
            {
                fmt = "png";
            }
            std::string name = "__embedded_tex_" + std::to_string(embIndex) + "." + fmt;
            std::filesystem::path outPath = modelFilePath / name;
            std::ofstream ofs(outPath, std::ios::binary);
            if (!ofs)
            {
                return "";
            }
            ofs.write(reinterpret_cast<const char *>(texture->pcData), static_cast<std::streamsize>(texture->mWidth));
            ofs.close();
            return outPath.string();
        }
        return "";
    }

    int ElecNekoWorld::ResolveAndLoadTextureFromMaterial(DeviceContext *device, aiMaterial *aim, const aiScene *scene, const std::filesystem::path &basePath,
                                                         const std::vector<aiTextureType> &tryTypes, ElecNekoWorld *world)
    {
        aiString texPath;
        for (auto type: tryTypes)
        {
            if (aim->GetTexture(type, 0, &texPath) == AI_SUCCESS)
            {
                std::string p = texPath.C_Str();
                if (p.empty())
                {
                    continue;
                }
                if (p.front() == '*')
                {
                    int emb = 0;
                    try
                    {
                        emb = std::stoi(p.substr(1));
                    } catch (...)
                    {
                        continue;
                    }

                    if (emb >= 0 && emb < static_cast<int>(scene->mNumTextures))
                    {
                        aiTexture *atex = scene->mTextures[emb];
                        std::string savedPath = SaveEmbeddedTextureToTempFile(atex, basePath, emb);
                        if (!savedPath.empty())
                        {
                            return world->EnsureTextureCached(device, savedPath);
                        }
                    }
                }
                else
                {
                    std::filesystem::path ppath(p);
                    std::filesystem::path resolved;
                    if (ppath.is_absolute())
                    {
                        resolved = ppath;
                    }
                    else
                    {
                        resolved = (basePath / ppath).lexically_normal();
                    }
                    return world->EnsureTextureCached(device, resolved.string());
                }
            }
        }
        return -1;
    }

    Material ElecNekoWorld::ConvertAssimpMaterial(DeviceContext *device, aiMaterial *aim, const aiScene *scene, const std::filesystem::path &basePath,
                                                  ElecNekoWorld *world)
    {
        Material mat;
        aiString name;
        if (aim->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
        {
            mat.name = name.C_Str();
        }

        aiColor4D diffuse(1.f, 1.f, 1.f, 1.f);
        if (aim->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS)
        {
            mat.baseColor = Vec3(diffuse.r, diffuse.g, diffuse.b);
            mat.opacity = diffuse.a;
        }

        float opacity = 1.f;
        if (aim->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
        {
            mat.opacity = opacity;
        }

        float metal = 0.f, rough = .5f;
        aim->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, metal);
        aim->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, rough);
        mat.metallic = metal;
        mat.roughness = sqrtf((rough > 0.f) ? rough : 0.f);

        aiColor3D emissive(0.f, 0.f, 0.f);
        if (aim->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
        {
            mat.emission = Vec3(emissive.r, emissive.g, emissive.b);
        }

        float alphaCutoff = 0.5f;
        if (aim->Get(AI_MATKEY_GLTF_ALPHACUTOFF, alphaCutoff) == AI_SUCCESS)
        {
            mat.alphaCutoff = alphaCutoff;
        }

        float specTrans = 0.f;
        if (aim->Get(AI_MATKEY_GLTF_MATERIAL_TRANSMISSION_FACTOR, specTrans) == AI_SUCCESS)
        {
            mat.specTrans = specTrans;
        }

        mat.alphaMode = AlphaMode::Opaque;
        uint32_t alphaMode = 0;
        if (aim->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS)
        {
            if (alphaMode == 1)
            {
                mat.alphaMode = AlphaMode::Mask;
            }
            else if (alphaMode == 2)
            {
                mat.alphaMode = AlphaMode::Blend;
            }
            else
            {
                mat.alphaMode = AlphaMode::Opaque;
            }
        }

        {
            std::vector<aiTextureType> trySlots = {aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR};
            int tid = ResolveAndLoadTextureFromMaterial(device, aim, scene, basePath, trySlots, world);
            if (tid >= 0)
            {
                mat.baseColorTexId = tid;
            }
        }
        {
            std::vector<aiTextureType> trySlots = {aiTextureType_NORMALS, aiTextureType_HEIGHT};
            int tid = ResolveAndLoadTextureFromMaterial(device, aim, scene, basePath, trySlots, world);
            if (tid >= 0)
            {
                mat.normalMapTexId = tid;
            }
        }
        {
            std::vector<aiTextureType> trySlots = {aiTextureType_GLTF_METALLIC_ROUGHNESS, aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS};
            int tid = ResolveAndLoadTextureFromMaterial(device, aim, scene, basePath, trySlots, world);
            if (tid >= 0)
            {
                mat.metallicRoughtnessTexId = tid;
            }
        }
        {
            std::vector<aiTextureType> trySlots = {aiTextureType_EMISSIVE};
            int tid = ResolveAndLoadTextureFromMaterial(device, aim, scene, basePath, trySlots, world);
            if (tid >= 0)
            {
                mat.emissionmapTexId = tid;
            }
        }

        if (mat.alphaMode == AlphaMode::Opaque)
        {
            if (mat.opacity < 0.999f)
            {
                mat.alphaMode = AlphaMode::Blend;
            }
            else if (mat.alphaCutoff < (1.f - 1e-4f))
            {
                mat.alphaMode = AlphaMode::Mask;
            }
        }

        return mat;
    }

    void ElecNekoWorld::TraverseAssimpNodesAndCreateInstance(const aiScene *scene, const aiNode *node, const aiMatrix4x4 &parentTransform,
                                                             const std::vector<int> &aiToWorldMesh, const std::vector<int> &aiToWorldMat, ElecNekoWorld *world)
    {
        if (!node)
        {
            return;
        }

        aiMatrix4x4 global = parentTransform * node->mTransformation;

        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
        {
            uint32_t aiMeshIndex = node->mMeshes[i];
            if (aiMeshIndex >= aiToWorldMesh.size())
            {
                continue;
            }
            int worldMeshIndex = aiToWorldMesh[aiMeshIndex];
            if (worldMeshIndex < 0 || worldMeshIndex >= static_cast<int>(world->m_models.size()))
            {
                continue;
            }

            int worldMat = 0;
            const aiMesh *am = scene->mMeshes[aiMeshIndex];
            if (am && am->mMaterialIndex >= 0 && am->mMaterialIndex < static_cast<int>(aiToWorldMat.size()))
            {
                worldMat = aiToWorldMat[am->mMaterialIndex];
            }

            InstanceCPU instance;
            instance.model = AiToMat4(global);
            instance.meshId = worldMeshIndex;
            instance.materialId = worldMat;
            world->m_instances.push_back(instance);
        }

        for (uint32_t c = 0; c < node->mNumChildren; ++c)
        {
            TraverseAssimpNodesAndCreateInstance(scene, node->mChildren[c], global, aiToWorldMesh, aiToWorldMat, world);
        }
    }

    int ElecNekoWorld::LoadModelGeometryOnly(DeviceContext *device, const std::string &filename, int overrideMaterialId)
    {
        std::string key = NormalizePathForKey(filename);
        auto it = m_modelIndexByKey.find(key);
        if (it != m_modelIndexByKey.end())
        {
            return it->second;
        }

        Assimp::Importer importer;
        unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
                             aiProcess_ImproveCacheLocality;
        const aiScene *scene = importer.ReadFile(filename, flags);
        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !scene->mRootNode)
        {
            printf("Failed to load model %s: %s\n", filename.c_str(), importer.GetErrorString());
            return -1;
        }

        ElecNekoModel *model = new ElecNekoModel();
        model->name = std::filesystem::path(filename).stem().string();

        size_t totalVerticesHint = 0;
        size_t totalFacesHint = 0;

        for (uint32_t mi = 0; mi < scene->mNumMeshes; ++mi)
        {
            totalVerticesHint += scene->mMeshes[mi]->mNumVertices;
            totalFacesHint += scene->mMeshes[mi]->mNumFaces;
        }

        model->m_vertices.reserve(totalVerticesHint);
        model->m_indices.reserve(totalFacesHint * 3);

        std::unordered_map<VVertex, uint32_t, VVertexHash> dedup;
        dedup.reserve((totalVerticesHint > 0) ? totalVerticesHint * 2 : 128);

        uint32_t indexBase = 0;
        for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
        {
            aiMesh *am = scene->mMeshes[mi];
            if (!am)
            {
                continue;
            }

            uint32_t subStartIndex = static_cast<uint32_t>(model->m_indices.size());

            for (uint32_t fi = 0; fi < am->mNumFaces; ++fi)
            {
                const aiFace &face = am->mFaces[fi];
                if (face.mNumIndices != 3)
                {
                    continue;
                }
                for (uint32_t k = 0; k < 3; ++k)
                {
                    uint32_t idx = face.mIndices[k];
                    VVertex vv{};
                    if (am->HasPositions())
                    {
                        vv.position[0] = am->mVertices[idx].x;
                        vv.position[1] = am->mVertices[idx].y;
                        vv.position[2] = am->mVertices[idx].z;
                    }
                    if (am->HasTextureCoords(0))
                    {
                        vv.uv[0] = am->mTextureCoords[0][idx].x;
                        vv.uv[1] = 1.f - am->mTextureCoords[0][idx].y;
                    }
                    if (am->HasNormals())
                    {
                        vv.normal[0] = am->mNormals[idx].x;
                        vv.normal[1] = am->mNormals[idx].y;
                        vv.normal[2] = am->mNormals[idx].z;
                    }

                    auto itv = dedup.find(vv);
                    if (itv == dedup.end())
                    {
                        uint32_t newIndex = static_cast<uint32_t>(model->m_vertices.size());
                        model->m_vertices.push_back(vv);
                        model->m_indices.push_back(newIndex);
                        dedup.emplace(vv, newIndex);
                    }
                    else
                    {
                        model->m_indices.push_back(itv->second);
                    }
                }
            }
            uint32_t subEndIndex = static_cast<uint32_t>(model->m_indices.size());
            SubMesh s;
            s.indexOffset = subStartIndex;
            s.indexCount = (subEndIndex > subStartIndex) ? (subEndIndex - subStartIndex) : 0;
            // TODO: maybe could be more robust
            s.materialId = overrideMaterialId;
            model->subMeshes.push_back(s);
        }

        if (model->m_vertices.empty() || model->m_indices.empty())
        {
            delete model;
            printf("Model %s has no valid geometry\n", filename.c_str());
            return -1;
        }

        if (!model->MakeVBO(device))
        {
            delete model;
            printf("Failed to create VBO for model %s\n", filename.c_str());
            return -1;
        }

        int firstIndex = static_cast<int>(m_models.size());
        m_models.push_back(model);
        m_modelIndexByKey.emplace(key, firstIndex);
        return firstIndex;
    }

    int ElecNekoWorld::LoadModelWithMaterials(DeviceContext *device, const std::string &filename, Mat4 transMat)
    {
        Assimp::Importer importer;

        unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
                             aiProcess_ImproveCacheLocality;

        const aiScene *scene = importer.ReadFile(filename, flags);
        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !scene->mRootNode)
        {
            std::cerr << "Assimp: Failed to load model " << filename << ": " << importer.GetErrorString() << std::endl;
            return -1;
        }

        std::filesystem::path basePath = std::filesystem::path(filename).parent_path();
        int firstIndex = static_cast<int>(m_models.size());


        std::vector<int> aiTOWorldMat(scene->mNumMaterials, -1);
        for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
        {
            aiMaterial *aim = scene->mMaterials[i];
            Material mat = ConvertAssimpMaterial(device, aim, scene, basePath, this);
            int wid = AddMaterial(mat);
            aiTOWorldMat[i] = wid;
        }
        if (m_materials.empty())
        {
            Material defaultMat;
            AddMaterial(defaultMat);
        }

        std::vector<int> aiToWorldMesh(scene->mNumMeshes, -1);
        for (uint32_t mi = 0; mi < scene->mNumMeshes; ++mi)
        {
            aiMesh *am = scene->mMeshes[mi];
            if (!am)
            {
                continue;
            }

            ElecNekoModel *model = new ElecNekoModel();
            model->name = (am->mName.C_Str() && am->mName.length) ? am->mName.C_Str()
                                                                  : (std::filesystem::path(filename).stem().string() + "_part" + std::to_string(mi));
            model->m_vertices.reserve(am->mNumVertices);
            model->m_indices.reserve(am->mNumFaces * 3);

            std::unordered_map<VVertex, uint32_t, VVertexHash> dedup;
            dedup.reserve((am->mNumVertices > 0) ? am->mNumVertices * 2 : 128);

            uint32_t subStartIndex = static_cast<uint32_t>(model->m_indices.size());
            for (uint32_t fi = 0; fi < am->mNumFaces; ++fi)
            {
                const aiFace &face = am->mFaces[fi];
                if (face.mNumIndices != 3)
                {
                    continue;
                }

                for (uint32_t k = 0; k < 3; ++k)
                {
                    uint32_t idx = face.mIndices[k];
                    VVertex vv{};
                    if (am->HasPositions())
                    {
                        vv.position[0] = am->mVertices[idx].x;
                        vv.position[1] = am->mVertices[idx].y;
                        vv.position[2] = am->mVertices[idx].z;
                    }
                    if (am->HasTextureCoords(0))
                    {
                        vv.uv[0] = am->mTextureCoords[0][idx].x;
                        vv.uv[1] = 1.f - am->mTextureCoords[0][idx].y;
                    }
                    if (am->HasNormals())
                    {
                        vv.normal[0] = am->mNormals[idx].x;
                        vv.normal[1] = am->mNormals[idx].y;
                        vv.normal[2] = am->mNormals[idx].z;
                    }
                    auto it = dedup.find(vv);
                    if (it == dedup.end())
                    {
                        uint32_t newIndex = static_cast<uint32_t>(model->m_vertices.size());
                        model->m_vertices.push_back(vv);
                        model->m_indices.push_back(newIndex);
                        dedup.emplace(vv, newIndex);
                    }
                    else
                    {
                        model->m_indices.push_back(it->second);
                    }
                }
            }
            uint32_t subEndIndex = static_cast<uint32_t>(model->m_indices.size());
            SubMesh s;
            s.indexOffset = subStartIndex;
            s.indexCount = (subEndIndex > subStartIndex) ? (subEndIndex - subStartIndex) : 0;
            s.materialId = 0;
            if (am && am->mMaterialIndex >= 0 && am->mMaterialIndex < static_cast<int>(aiTOWorldMat.size()))
            {
                s.materialId = aiTOWorldMat[am->mMaterialIndex];
            }
            model->subMeshes.push_back(s);

            if (!model->MakeVBO(device))
            {
                delete model;
                std::cerr << "Assimp: Failed to create VBO for model " << filename << std::endl;
                continue;
            }
            int worldMeshIndex = static_cast<int>(m_models.size());
            m_models.push_back(model);
            aiToWorldMesh[mi] = worldMeshIndex;
        }

        aiMatrix4x4 trans = Mat4ToAiMatrix4x4(transMat);
        TraverseAssimpNodesAndCreateInstance(scene, scene->mRootNode, trans, aiToWorldMesh, aiTOWorldMat, this);

        if (scene->mNumMeshes == 0)
        {
            std::cerr << "Assimp: Model " << filename << " has no meshes." << std::endl;
            return -1;
        }
        return firstIndex;
    }

    int ElecNekoWorld::EnsureTextureCached(DeviceContext *device, const std::string &filename)
    {
        std::string key = NormalizePathForKey(filename);
        auto it = m_textureChace.find(key);
        if (it != m_textureChace.end())
        {
            return it->second;
        }
        int texId = AddTexture(device, filename);
        if (texId >= 0)
        {
            m_textureChace.emplace(key, texId);
        }
        return texId;
    }

    int ElecNekoWorld::AddTexture(DeviceContext *device, const std::string &filename)
    {
        Texture *texture = new Texture();
        if (!texture->LoadTexture(device, filename))
        {
            delete texture;
            std::cerr << "Failed to load texture " << filename << std::endl;
            return -1;
        }
        int idx = static_cast<int>(m_textures.size());
        m_textures.push_back(texture);
        return idx;
    }

    int ElecNekoWorld::AddMaterial(const Material &material)
    {
        int idx = static_cast<int>(m_materials.size());
        m_materials.push_back(material);
        return idx;
    }

    void ElecNekoWorld::AddCamera(const Vec3 &pos, const Vec3 &lookAt, float fov, float aspectRatio, float zNear, float zFar)
    {
        if (m_cam)
        {
            delete m_cam;
        }

        m_cam = new Camera;
        m_cam->Initialize(pos, lookAt, fov, aspectRatio, zNear, zFar);
    }

    void ElecNekoWorld::CreateDefaultTextures(DeviceContext *device)
    {
        std::array<uint8_t, 4> whitePixel = {255, 255, 255, 255};
        defaultAlbedo = new Texture("default_albedo", whitePixel.data(), 1, 1, 4);

        std::array<uint8_t, 4> normalPixel = {128, 128, 255, 255};
        defaultNormal = new Texture("default_normal", normalPixel.data(), 1, 1, 4);

        std::array<uint8_t, 4> metalRoughPixel = {0, 255, 0, 255};
        defaultMetalRough = new Texture("default_metalrough", metalRoughPixel.data(), 1, 1, 4);

        std::array<uint8_t, 4> emissionPixel = {0, 0, 0, 255};
        defaultEmission = new Texture("default_emission", emissionPixel.data(), 1, 1, 4);
    }

    bool ElecNekoWorld::BuildInstanceAndIndirectBuffers(DeviceContext *device)
    {
        // group instances by meshId
        meshIndirectInfos.clear();
        indirectCount = 0;

        std::vector<InstanceGPU> gpuInstances;
        std::vector<VkDrawIndexedIndirectCommand> indirectCommands;

        if (m_instances.empty())
        {
            instanceBuffer.Cleanup(device);
            indirectBuffer.Cleanup(device);

            return true;
        }

        std::unordered_map<int, std::vector<int>> instancesByMesh;
        for (size_t i = 0; i < m_instances.size(); ++i)
        {
            instancesByMesh[m_instances[i].meshId].push_back(static_cast<int>(i));
        }

        // for building meshIndirectInfos we need to know per-mesh start offset in indirectCmds
        for (auto &entry: instancesByMesh)
        {
            int meshId = entry.first;
            ElecNekoModel *model = (meshId > 0 && meshId < static_cast<int>(m_models.size())) ? m_models[meshId] : nullptr;
            if (!model)
            {
                continue;
            }
            MeshIndirectInfo info;
            info.indirectOffsetInBytes = static_cast<uint32_t>(indirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand));
            info.drawCount = 0;

            // split instances into overrides (instance.materialId >=0) and non-overridden (-1)
            std::unordered_map<int, std::vector<int>> overridesByMat;
            std::vector<int> nonOverrides;
            for (int instIdx: entry.second)
            {
                const InstanceCPU &inst = m_instances[instIdx];
                if (inst.materialId >= 0)
                {
                    overridesByMat[inst.materialId].push_back(instIdx);
                }
                else
                {
                    nonOverrides.push_back(instIdx);
                }
            }

            // handle overrides: each override group becomes instance block, and for each submesh we will emit a draw using the override material
            for (auto &ovEntry: overridesByMat)
            {
                const auto &instIndices = ovEntry.second;
                if (instIndices.empty())
                {
                    continue;
                }
                uint32_t firstInstance = static_cast<uint32_t>(gpuInstances.size());
                for (int ii: instIndices)
                {
                    InstanceGPU g;
                    // copy Mat4
                    const Mat4 &m = m_instances[ii].model;
                    g.modelRow0[0] = m.rows[0].x;
                    g.modelRow0[1] = m.rows[0].y;
                    g.modelRow0[2] = m.rows[0].z;
                    g.modelRow0[3] = m.rows[0].w;

                    g.modelRow1[0] = m.rows[1].x;
                    g.modelRow1[1] = m.rows[1].y;
                    g.modelRow1[2] = m.rows[1].z;
                    g.modelRow1[3] = m.rows[1].w;

                    g.modelRow2[0] = m.rows[2].x;
                    g.modelRow2[1] = m.rows[2].y;
                    g.modelRow2[2] = m.rows[2].z;
                    g.modelRow2[3] = m.rows[2].w;

                    g.modelRow3[0] = m.rows[3].x;
                    g.modelRow3[1] = m.rows[3].y;
                    g.modelRow3[2] = m.rows[3].z;
                    g.modelRow3[3] = m.rows[3].w;

                    g.materialId = static_cast<uint32_t>(m_instances[ii].materialId);
                    if (g.materialId >= m_materials.size() || g.materialId < 0)
                    {
                        g.materialId = 0;
                    }
                    g.padding[0] = g.padding[1] = g.padding[2] = 0;
                    gpuInstances.push_back(g);
                }

                // emit one draw for each submesh (drawCount increases)
                for (const auto &sub: model->subMeshes)
                {
                    VkDrawIndexedIndirectCommand cmd{};
                    cmd.indexCount = sub.indexCount;
                    cmd.instanceCount = static_cast<uint32_t>(instIndices.size());
                    cmd.firstIndex = sub.indexOffset;
                    cmd.vertexOffset = 0;
                    cmd.firstInstance = firstInstance;
                    indirectCommands.push_back(cmd);
                    info.drawCount++;
                }
            }

            // handle non-overridden: create single instance block used for all submeshes, where material is submesh.materialId
            if (!nonOverrides.empty())
            {
                uint32_t firstInstance = static_cast<uint32_t>(gpuInstances.size());
                for (int ii: nonOverrides)
                {
                    InstanceGPU g;
                    const Mat4 &m = m_instances[ii].model;
                    g.modelRow0[0] = m.rows[0].x;
                    g.modelRow0[1] = m.rows[0].y;
                    g.modelRow0[2] = m.rows[0].z;
                    g.modelRow0[3] = m.rows[0].w;

                    g.modelRow1[0] = m.rows[1].x;
                    g.modelRow1[1] = m.rows[1].y;
                    g.modelRow1[2] = m.rows[1].z;
                    g.modelRow1[3] = m.rows[1].w;

                    g.modelRow2[0] = m.rows[2].x;
                    g.modelRow2[1] = m.rows[2].y;
                    g.modelRow2[2] = m.rows[2].z;
                    g.modelRow2[3] = m.rows[2].w;

                    g.modelRow3[0] = m.rows[3].x;
                    g.modelRow3[1] = m.rows[3].y;
                    g.modelRow3[2] = m.rows[3].z;
                    g.modelRow3[3] = m.rows[3].w;

                    g.materialId = 0; // non-overridden -> shader should index mesh.submesh.materialId instead

                    g.padding[0] = g.padding[1] = g.padding[2] = 0;
                    gpuInstances.push_back(g);
                }
                for (const auto &s: model->subMeshes)
                {
                    VkDrawIndexedIndirectCommand cmd{};
                    cmd.indexCount = s.indexCount;
                    cmd.instanceCount = static_cast<uint32_t>(nonOverrides.size());
                    cmd.firstIndex = s.indexOffset;
                    cmd.vertexOffset = 0;
                    cmd.firstInstance = firstInstance;
                    indirectCommands.push_back(cmd);
                    info.drawCount++;
                }
            }

            meshIndirectInfos[meshId] = info;
        }

        if (!gpuInstances.empty())
        {
            VkDeviceSize sz = gpuInstances.size() * sizeof(InstanceGPU);
            instanceBuffer.Cleanup(device);
            if (!instanceBuffer.Allocate(device, gpuInstances.data(), static_cast<int>(sz), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
            {
                printf("Failed to allocate instanceBuffer\n");
                return false;
            }
        }
        else
        {
            instanceBuffer.Cleanup(device);
        }

        // upload indirectBuffer
        if (!indirectCommands.empty())
        {
            VkDeviceSize sz = indirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand);
            indirectBuffer.Cleanup(device);
            if (!indirectBuffer.Allocate(device, indirectCommands.data(), static_cast<int>(sz), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
            {
                printf("Failed to allocate indirectBuffer\n");
                return false;
            }
            indirectCount = static_cast<uint32_t>(indirectCommands.size());
        }
        else
        {
            indirectBuffer.Cleanup(device);
            indirectCount = 0;
        }

        return true;
    }

    bool ElecNekoWorld::BuildMaterialBuffers(DeviceContext *device)
    {
        // Build material buffer from materials vector
        if (!m_materials.empty())
        {
            // Convert Materials to Material_t for GPU
            std::vector<Material_t> materialStructs;
            materialStructs.reserve(m_materials.size());

            for (auto &mat: m_materials)
            {
                materialStructs.push_back(mat.MakeStrcut());
            }

            // Clean up old buffer if exists
            materialBuffer.Cleanup(device);

            // Allocate new material buffer
            VkDeviceSize bufferSize = materialStructs.size() * sizeof(Material_t);
            if (!materialBuffer.Allocate(device, materialStructs.data(), static_cast<int>(bufferSize), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
            {
                printf("Failed to allocate material buffer!\n");
                return false;
            }
        }

        // Build texture array from loaded textures
        if (!m_textures.empty() && textureArray == nullptr)
        {
            textureArray = new TextureArray();

            // Collect texture data
            std::vector<TextureProperty> textureProperties;
            textureProperties.reserve(m_textures.size());

            for (const auto *tex: m_textures)
            {
                if (tex && tex->isLoaded)
                {
                    textureProperties.push_back(tex->ExtractProperties());
                }
            }

            // Create texture array
            if (!textureProperties.empty())
            {
                // Assuming all textures have the same dimensions, get from first texture
                int texWidth = m_textures[0]->width;
                int texHeight = m_textures[0]->height;
                int texComponents = m_textures[0]->components;

                if (!textureArray->CreateFromData(device, textureProperties, texWidth, texHeight, texComponents, "material_textures"))
                {
                    printf("Failed to create texture array!\n");
                    delete textureArray;
                    textureArray = nullptr;
                    return false;
                }
            }
        }

        return true;
    }

    // void ElecNekoWorld::DrawIndirect(VkCommandBuffer cmd, std::function<void(int)> bindMaterialFunc)
    void ElecNekoWorld::DrawIndirect(RHICommandList &cmd, std::function<void(int)> bindMaterialFunc)
    {
        if (indirectCount == 0)
        {
            return;
        }

        // For each mesh, bind that mesh's vertex/index buffers plus global instanceBuffer (binding 1)
        for (auto &mi: meshIndirectInfos)
        {
            int meshId = mi.first;
            MeshIndirectInfo &info = mi.second;

            ElecNekoModel *model = (meshId >= 0 && meshId < static_cast<int>(m_models.size())) ? m_models[meshId] : nullptr;
            if (!model)
            {
                continue;
            }

            // binding vertex buffers: binding 0 = mesh vertex buffer, binding 1 = instance buffer
            // VkBuffer vbs[2] = {model->m_vertexBuffer.m_vkBuffer, instanceBuffer.m_vkBuffer};
            // VkDeviceSize offsets[2] = {0, 0};
            // vkCmdBindVertexBuffers(cmd, 0, 2, vbs, offsets);

            // // bind index buffer
            // vkCmdBindIndexBuffer(cmd, model->m_indexBuffer.m_vkBuffer, 0, VK_INDEX_TYPE_UINT32);
            VkBuffer vbs[] = {model->m_vertexBuffer.m_vkBuffer, instanceBuffer.m_vkBuffer};

            VkDeviceSize offsets[] = {model->m_vertexBuffer.m_offset, instanceBuffer.m_offset};

            cmd.SetVertexBuffers(0, 2, vbs, offsets);
            cmd.SetIndexBuffer(model->m_indexBuffer, RHIIndexFormat::UInt32);

            // We must issue vkCmdDrawIndexedIndirect for this mesh's section
            // However if you need per-submesh material descriptor bind before each sub-draw, you can provide bindMaterialFn to do that.
            // To support that, we need to iterate per submesh and call vkCmdDrawIndexedIndirect for each submesh separately.
            // Our meshIndirectInfo stores the offset in the combined indirectBuffer and number of draws for this mesh, but we don't
            // record per-submesh mapping in detail here. So we'll assume the following simple approach:
            // - If bindMaterialFn==nullptr: we call one vkCmdDrawIndexedIndirect covering all commands for this mesh at once.
            // - If bindMaterialFn provided: we will iterate mesh->m_submeshes and call vkCmdDrawIndexedIndirect per submesh,
            //   with offset computed by scanning earlier commands (we did not store per-submesh offsets). To keep code simple and robust,
            //   we'll take the pragmatic route: if bindMaterialFn != nullptr, we issue per-submesh draws using the assumption that
            //   when building indirectBuffer we appended commands in the same order as mesh->m_submeshes (which is true in our builder).
            //
            // Therefore we need to compute subOffsetBytes for this mesh. The builder created commands for each mesh contiguously:
            //   meshIndirectInfos[meshId].indirectOffsetInBytes points to the first command for this mesh
            // So for submesh i (0..N-1) the command offset = base + i * sizeof(VkDrawIndexedIndirectCommand) �� BUT note overrides create more commands
            // For correctness we will use a simple per-command iteration counting style: iterate drawCount and for each draw call bindMaterialFn with
            //   either override material or submesh material depending on how builder made commands.
            // To keep this safe, we'll implement the following:
            // If bindMaterialFn==nullptr -> single bulk vkCmdDrawIndexedIndirect.
            // If bindMaterialFn!=nullptr -> call vkCmdDrawIndexedIndirect per-draw and call bindMaterialFn with material id:
            //   - The builder arranged commands such that overrides for a mesh come first (grouped by override material),
            //     then non-overridden commands (one per submesh). We don't store per-command material here; if you need per-command material,
            //     extend MeshIndirectInfo to carry an array of materialIds per draw. For now we'll call bindMaterialFn(-1) as a hint when we can't determine
            //     material.
            //
            // Simpler: if you need per-draw descriptor binding, I recommend modifying BuildInstanceAndIndirectBuffers to also append a parallel vector
            // of materialIds (one per indirect command). The code below will assume no materialIds vector exists and will call bindMaterialFn(-1).
            //
            if (!bindMaterialFunc)
            {
                // one big multi-draw for this mesh
                // vkCmdDrawIndexedIndirect(cmd, indirectBuffer.m_vkBuffer, info.indirectOffsetInBytes, info.drawCount, sizeof(VkDrawIndexedIndirectCommand));
                cmd.DrawIndexedIndirect(indirectBuffer, info.indirectOffsetInBytes, info.drawCount, sizeof(VkDrawIndexedIndirectCommand));
            }
            else
            {
                // per-draw loop (we don't have per-command material IDs stored here, so we pass -1).
                // If you want correct per-draw binding, modify BuildInstanceAndIndirectBuffers to also fill vector<uint32_t> indirectMaterialIds parallel to
                // indirectCmds.
                uint32_t stride = static_cast<uint32_t>(sizeof(VkDrawIndexedIndirectCommand));
                for (uint32_t di = 0; di < info.drawCount; ++di)
                {
                    bindMaterialFunc(-1); // placeholder: bind generic / fallback descriptor or use material SSBO.
                    // vkCmdDrawIndexedIndirect(cmd, indirectBuffer.m_vkBuffer, info.indirectOffsetInBytes + di * stride, 1,
                    // sizeof(VkDrawIndexedIndirectCommand));
                    cmd.DrawIndexedIndirect(indirectBuffer, info.indirectOffsetInBytes + di * stride, 1, sizeof(VkDrawIndexedIndirectCommand));
                }
            }
        }
    }

    void ElecNekoWorld::UnloadScene(DeviceContext *device)
    {
        m_instances.clear();

        for (auto *model: m_models)
        {
            if (model)
            {
                model->Cleanup(device);
                delete model;
            }
        }
        m_models.clear();
        m_modelIndexByKey.clear();

        for (auto *tex: m_textures)
        {
            if (tex)
            {
                tex->Cleanup(device);
                delete tex;
            }
        }
        m_textures.clear();
        m_textureChace.clear();

        m_materials.clear();

        if (m_cam)
        {
            delete m_cam;
            m_cam = nullptr;
        }

        instanceBuffer.Cleanup(device);
        indirectBuffer.Cleanup(device);
        meshIndirectInfos.clear();
        indirectCount = 0;

        // Clean up material buffer and texture array
        materialBuffer.Cleanup(device);
        if (textureArray)
        {
            textureArray->Cleanup(device);
            delete textureArray;
            textureArray = nullptr;
        }
    }

    void ElecNekoWorld::Cleanup(DeviceContext *device)
    {
        UnloadScene(device);
        if (defaultAlbedo)
        {
            defaultAlbedo->Cleanup(device);
            delete defaultAlbedo;
            defaultAlbedo = nullptr;
        }
        if (defaultNormal)
        {
            defaultNormal->Cleanup(device);
            delete defaultNormal;
            defaultNormal = nullptr;
        }
        if (defaultMetalRough)
        {
            defaultMetalRough->Cleanup(device);
            delete defaultMetalRough;
            defaultMetalRough = nullptr;
        }
        if (defaultEmission)
        {
            defaultEmission->Cleanup(device);
            delete defaultEmission;
            defaultEmission = nullptr;
        }
    }

    bool ElecNekoWorld::LoadSceneFromFile(DeviceContext *device, const std::string &filename)
    {
        // UnloadScene(device);

        std::ifstream ifs(filename);
        if (!ifs.is_open())
        {
            std::cerr << "Failed to open scene file: " << filename << std::endl;
            return false;
        }

        std::filesystem::path basePath = std::filesystem::path(filename).parent_path();

        // material name -> world material id
        std::unordered_map<std::string, int> materialMap;

        std::string line;
        while (std::getline(ifs, line))
        {
            auto t = Trim(line);
            if (t.empty() || t.front() == '#')
            {
                continue;
            }

            std::istringstream iss(t);
            std::string token;
            iss >> token;

            if (token == "material")
            {
                std::string materialName;
                std::string nextToken;
                if (iss >> nextToken)
                {
                    if (nextToken != "{")
                    {
                        materialName = nextToken;
                    }
                    else
                    {
                        materialName.clear();
                    }
                }

                auto block = ReadBlock(ifs, t);
                Material mat;
                for (auto &ln: block)
                {
                    if (ln.rfind("color", 0) == 0)
                    {
                        if (auto v = ParseVec3(ln))
                        {
                            mat.baseColor = Vec3((*v)[0], (*v)[1], (*v)[2]);
                        }
                    }
                    else if (ln.rfind("opacity", 0) == 0)
                    {
                        mat.opacity = ParseSingleFloat(ln, mat.opacity);
                    }
                    else if (ln.rfind("alphamode", 0) == 0)
                    {
                        auto s = ParseSingleToken(ln);
                        if (s == "mask")
                        {
                            mat.alphaMode = AlphaMode::Mask;
                        }
                        else if (s == "blend")
                        {
                            mat.alphaMode = AlphaMode::Blend;
                        }
                        else
                        {
                            mat.alphaMode = AlphaMode::Opaque;
                        }
                    }
                    else if (ln.rfind("alphacutoff", 0) == 0)
                    {
                        mat.alphaCutoff = ParseSingleFloat(ln, mat.alphaCutoff);
                    }
                    else if (ln.rfind("albedotexture", 0) == 0)
                    {
                        auto tok = ParseSingleToken(ln);
                        if (!tok.empty() && tok != "none")
                        {
                            mat.baseColorTexId = EnsureTextureCached(device, (basePath / tok).string());
                        }
                    }
                    else if (ln.rfind("normaltexture", 0) == 0)
                    {
                        auto tok = ParseSingleToken(ln);
                        if (!tok.empty() && tok != "none")
                        {
                            mat.normalMapTexId = EnsureTextureCached(device, (basePath / tok).string());
                        }
                    }
                    else if (ln.rfind("metallicRoughnesstexture", 0) == 0)
                    {
                        auto tok = ParseSingleToken(ln);
                        if (!tok.empty() && tok != "none")
                        {
                            mat.metallicRoughtnessTexId = EnsureTextureCached(device, (basePath / tok).string());
                        }
                    }
                    else if (ln.rfind("emissiontexture", 0) == 0)
                    {
                        auto tok = ParseSingleToken(ln);
                        if (!tok.empty() && tok != "none")
                        {
                            mat.emissionmapTexId = EnsureTextureCached(device, (basePath / tok).string());
                        }
                    }
                    else if (ln.rfind("emission", 0) == 0)
                    {
                        if (auto v = ParseVec3(ln))
                        {
                            mat.emission = Vec3((*v)[0], (*v)[1], (*v)[2]);
                        }
                    }
                    else if (ln.rfind("roughness", 0) == 0)
                    {
                        mat.roughness = ParseSingleFloat(ln, mat.roughness);
                    }
                    else if (ln.rfind("metallic", 0) == 0)
                    {
                        mat.metallic = ParseSingleFloat(ln, mat.metallic);
                    }
                    else if (ln.rfind("mediumtype", 0) == 0)
                    {
                        auto tok = ParseSingleToken(ln);
                        if (tok == "absorb")
                        {
                            mat.mediumType = MediumType::Absorb;
                        }
                        else if (tok == "scatter")
                        {
                            mat.mediumType = MediumType::Scatter;
                        }
                        else if (tok == "emissive")
                        {
                            mat.mediumType = MediumType::Emissive;
                        }
                    }
                    else if (ln.rfind("mediumdensity", 0) == 0)
                    {
                        mat.mediumDensity = ParseSingleFloat(ln, mat.mediumDensity);
                    }
                    else if (ln.rfind("mediumcolor", 0) == 0)
                    {
                        if (auto v = ParseVec3(ln))
                        {
                            mat.mediumColor = Vec3((*v)[0], (*v)[1], (*v)[2]);
                        }
                    }
                    else if (ln.rfind("anisotropic", 0) == 0)
                    {
                        mat.anisotropic = ParseSingleFloat(ln, mat.anisotropic);
                    }
                    else if (ln.rfind("subsurface", 0) == 0)
                    {
                        mat.subsurface = ParseSingleFloat(ln, mat.subsurface);
                    }
                    else if (ln.rfind("speculartint", 0) == 0)
                    {
                        mat.specularTint = ParseSingleFloat(ln, mat.specularTint);
                    }
                    else if (ln.rfind("sheen", 0) == 0)
                    {
                        mat.sheen = ParseSingleFloat(ln, mat.sheen);
                    }
                    else if (ln.rfind("sheentint", 0) == 0)
                    {
                        mat.sheenTint = ParseSingleFloat(ln, mat.sheenTint);
                    }
                    else if (ln.rfind("clearcoat", 0) == 0)
                    {
                        mat.clearcoat = ParseSingleFloat(ln, mat.clearcoat);
                    }
                    else if (ln.rfind("clearcoatgloss", 0) == 0)
                    {
                        mat.clearcoatGloss = ParseSingleFloat(ln, mat.clearcoatGloss);
                    }
                    else if (ln.rfind("specTrans", 0) == 0)
                    {
                        mat.specTrans = ParseSingleFloat(ln, mat.specTrans);
                    }
                    else if (ln.rfind("ior", 0) == 0)
                    {
                        mat.ior = ParseSingleFloat(ln, mat.ior);
                    }
                    else if (ln.rfind("mediumanisotropy", 0) == 0)
                    {
                        mat.mediumAnisotropy = ParseSingleFloat(ln, mat.mediumAnisotropy);
                    }
                    else if (ln.rfind("name", 0) == 0)
                    {
                        if (materialName.empty())
                        {
                            auto tok = ParseSingleToken(ln);
                            if (!tok.empty())
                            {
                                materialName = tok;
                            }
                        }
                    }
                }

                // register material
                mat.name = materialName;
                if (materialMap.find(materialName) == materialMap.end())
                {
                    int matId = AddMaterial(mat);
                    materialMap.emplace(materialName, matId);
                }

                continue;
            } // material

            if (token == "mesh")
            {
                auto block = ReadBlock(ifs, t);
                std::string fileTok;
                std::string meshName;
                std::string matName;
                Mat4 translate, scale, rotation;
                translate.Identity();
                scale.Identity();
                rotation.Identity();

                for (auto &ln: block)
                {
                    if (ln.rfind("file", 0) == 0)
                    {
                        fileTok = ParseSingleToken(ln);
                    }
                    else if (ln.rfind("name", 0) == 0)
                    {
                        meshName = ParseSingleToken(ln);
                    }
                    else if (ln.rfind("material", 0) == 0)
                    {
                        matName = ParseSingleToken(ln);
                    }
                    else if (ln.rfind("position", 0) == 0)
                    {
                        if (auto v = ParseVec3(ln))
                        {
                            translate.rows[3] = Vec4((*v)[0], (*v)[1], (*v)[2], 1.f);
                        }
                    }
                    else if (ln.rfind("scale", 0) == 0)
                    {
                        if (auto v = ParseVec3(ln))
                        {
                            scale.rows[0].x = (*v)[0];
                            scale.rows[1].y = (*v)[1];
                            scale.rows[2].z = (*v)[2];
                        }
                    }
                    else if (ln.rfind("rotation", 0) == 0)
                    {
                        std::istringstream iss2(ln);
                        std::string key;
                        if (iss2 >> key)
                        {
                            Quat rotQ;
                            if (iss2 >> rotQ.x >> rotQ.y >> rotQ.z >> rotQ.w)
                            {
                                rotation = rotQ.ToMat4();
                            }
                        }
                    }
                }

                if (!fileTok.empty())
                {
                    std::string full = (basePath / fileTok).lexically_normal().string();
                    int overrideMaterialId = 0;
                    if (!matName.empty() && materialMap.count(matName))
                    {
                        overrideMaterialId = materialMap[matName];
                    }

                    std::string ext = std::filesystem::path(full).extension().string();
                    for (auto &c: ext)
                    {
                        c = static_cast<char>(std::tolower((unsigned char) c));
                    }

                    Mat4 transform = scale * rotation * translate;

                    int meshIdx = LoadModelGeometryOnly(device, full, overrideMaterialId);
                    if (meshIdx >= 0)
                    {
                        std::string instanceName = (meshName.empty()) ? std::filesystem::path(fileTok).stem().string() : meshName;
                        instanceName += "_insatnce";
                        InstanceCPU inst{};
                        inst.model = transform;
                        inst.meshId = meshIdx;
                        inst.materialId = overrideMaterialId;
                        m_instances.push_back(inst);
                    }
                    else
                    {
                        std::cerr << "Failed to Load Model: " << full << "\n";
                    }
                }
                continue;
            } // mesh

            if (token == "gltf" || token == "obj" || token == "glb")
            {
                auto block = ReadBlock(ifs, t);
                std::string fileTok;
                Mat4 translate, scale, rotation;
                translate.Identity();
                scale.Identity();
                rotation.Identity();

                for (auto &ln: block)
                {
                    if (ln.rfind("file", 0) == 0)
                    {
                        fileTok = ParseSingleToken(ln);
                    }
                    else if (ln.rfind("position", 0) == 0)
                    {
                        if (auto v = ParseVec3(ln))
                        {
                            translate.rows[3] = Vec4((*v)[0], (*v)[1], (*v)[2], 1.f);
                        }
                    }
                    else if (ln.rfind("scale", 0) == 0)
                    {
                        if (auto v = ParseVec3(ln))
                        {
                            scale.rows[0].x = (*v)[0];
                            scale.rows[1].y = (*v)[1];
                            scale.rows[2].z = (*v)[2];
                        }
                    }
                    else if (ln.rfind("rotation", 0) == 0)
                    {
                        std::istringstream iss2(ln);
                        std::string key;
                        if (iss2 >> key)
                        {
                            Quat rotQ;
                            if (iss2 >> rotQ.x >> rotQ.y >> rotQ.z >> rotQ.w)
                            {
                                rotation = rotQ.ToMat4();
                            }
                        }
                    }
                }

                if (!fileTok.empty())
                {
                    std::string full = (basePath / fileTok).lexically_normal().string();
                    Mat4 transform = scale * rotation * translate;
                    LoadModelWithMaterials(device, full, transform);
                }
                continue;
            }

            if (token == "camera")
            {
                auto block = ReadBlock(ifs, t);
                Vec3 pos, lookAt;
                float fov = 45.f, aperture = 0.f, focalDist = 1.f;
                for (auto &ln: block)
                {
                    if (ln.rfind("position", 0) == 0)
                    {
                        if (auto v = ParseVec3(ln))
                        {
                            pos = Vec3((*v)[0], (*v)[1], (*v)[2]);
                        }
                    }
                    else if (ln.rfind("lookat", 0) == 0)
                    {
                        if (auto v = ParseVec3(ln))
                        {
                            lookAt = Vec3((*v)[0], (*v)[1], (*v)[2]);
                        }
                    }
                    else if (ln.rfind("fov", 0) == 0)
                    {
                        fov = ParseSingleFloat(ln, fov);
                    }
                    else if (ln.rfind("aperture", 0) == 0)
                    {
                        aperture = ParseSingleFloat(ln, aperture);
                    }
                    else if (ln.rfind("focaldist", 0) == 0)
                    {
                        focalDist = ParseSingleFloat(ln, focalDist);
                    }
                }
                AddCamera(pos, lookAt, fov);
                if (m_cam)
                {
                    m_cam->aperture = aperture;
                    m_cam->focalDist = focalDist;
                }
                continue;
            }
        }

        // Build GPU buffers after all resources are loaded
        if (!BuildMaterialBuffers(device))
        {
            std::cerr << "Failed to build material buffers!" << std::endl;
            return false;
        }

        if (!BuildInstanceAndIndirectBuffers(device))
        {
            std::cerr << "Failed to build instance and indirect buffers!" << std::endl;
            return false;
        }

        return true;
    }
} // namespace ElecNeko
