#include "World.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "assimp/pbrmaterial.h"

namespace ElecNeko
{
    static Vec3 AiColorToVec3(const aiColor4D &c) 
	{ 
		return Vec3(static_cast<float>(c.r), static_cast<float>(c.g), static_cast<float>(c.b));
	}

	static bool AiGetMaterialFloat(aiMaterial* m, const char* key, unsigned int type, unsigned int idx, float& out)
	{
        aiReturn r = m->Get(key, type, idx, out);
        return r == AI_SUCCESS;
	}

	static std::string GetAiTexturePath(const aiString &s) 
	{ 
		return std::string(s.C_Str());
	}

    // safe lowercase helper
    static std::string ToLowerStr(const std::string& s)
    { 
        std::string out = s;
        for (char& c : out)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return out;
    }

    bool ElecNekoMesh::MakeVBO(DeviceContext *device)
	{ 
		if (m_vertices.empty())
		{
            return true;
		}

		int bufferSize = static_cast<int>(sizeof(VVertex) * m_vertices.size());
        if (!m_vertexBuffer.Allocate(device, m_vertices.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
		{
            printf("Failed to Allocate vertex Buffer!\n");
            assert(0);
            return false;
		}

		bufferSize = static_cast<int>(sizeof(uint32_t) * m_indices.size());
        if (!m_indexBuffer.Allocate(device, m_indices.data(), bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
		{
            printf("Failed to Allocate  indices Buffer!\n");
            assert(0);
            return false;
		}
        return true;
	}

	void ElecNekoMesh::Cleanup(DeviceContext* device)
	{ 
		m_vertexBuffer.Cleanup(device);
        m_indexBuffer.Cleanup(device);
	}
	
	bool ElecNekoMeshInstance::MakeUBO(DeviceContext* device)
	{ 
		int bufferSize = sizeof(transform);
		if (!uniformBuffer.Allocate(device, transform.ToPtr(), bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
		{
            printf("Failed to allocate uniform buffer!\n");
            assert(0);
            return false;
		}

		return true;
	}

	void ElecNekoMeshInstance::Cleanup(DeviceContext* device) 
	{ 
		uniformBuffer.Cleanup(device);
	}

    void ElecNekoMesh::DrawIndexed(VkCommandBuffer vkCommandBuffer)
    {
        VkBuffer vertexBuffers[] = {m_vertexBuffer.m_vkBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(vkCommandBuffer, m_indexBuffer.m_vkBuffer, 0, VK_INDEX_TYPE_UINT32);

        // issue draw command
        vkCmdDrawIndexed(vkCommandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
    }

	static std::string Trim(std::string_view v)
	{
		size_t b = 0, e = v.size();
		while (b < e && std::isspace((unsigned char)v[b]))
		{
            ++b;
		}
		while (e > b && std::isspace((unsigned char)v[e - 1]))
		{
			--e;
		}
        return std::string(v.substr(b, e - b));
	}

	static std::vector<std::string> ReadBlock(std::ifstream& ifs, const std::string& firstLine)
	{
        std::vector<std::string> out;

        // If the opening line contains '{' then we start reading subsequent lines until matching '}'
        // If it does not, search for the next line with '{' then read.
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
                    continue;
            }
            if (!inBlock)
                return out;
        }

        std::string line;
        while (std::getline(ifs, line))
        {
            std::string t = Trim(line);
            if (t.empty())
                continue;
            if (t.find('}') != std::string::npos)
                break;
            out.push_back(t);
        }

        return out;
	}

	// parse a vec3 from a single line token like: "position -0.1 0.2 1.0"
    static std::optional<std::array<float, 3>> ParseVec3(const std::string &line)
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

    // parse two ints
    static std::optional<std::pair<int, int>> ParseTwoInts(const std::string &line)
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

    static std::string ParseSingleToken(const std::string &line)
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
        while (b < e && std::isspace((unsigned char)rest[b]))
        {
            ++b;
        }
        while (e > b && std::isspace((unsigned char)rest[e - 1]))
        {
            --e;
        }
        return std::string(rest.substr(b, e - b));
    }

    static float ParseSingleFloat(const std::string &line, float fallback = 0.0f)
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

    static std::string NormalizePathForKey(const std::string& p)
    {
        try
        {
            return std::filesystem::weakly_canonical(std::filesystem::path(p)).string();
        } catch (...)
        {
            return p;
        }
    }

    // helper: convert aiMatrix 4x4 ->Mat4
    static Mat4 AiToMat4(const aiMatrix4x4& m)
    { 
        Mat4 out;
        out.rows[0] = Vec4(static_cast<float>(m.a1), static_cast<float>(m.a2), static_cast<float>(m.a3), static_cast<float>(m.a4));
        out.rows[1] = Vec4(static_cast<float>(m.b1), static_cast<float>(m.b2), static_cast<float>(m.b3), static_cast<float>(m.b4));
        out.rows[2] = Vec4(static_cast<float>(m.c1), static_cast<float>(m.c2), static_cast<float>(m.c3), static_cast<float>(m.c4));
        out.rows[3] = Vec4(static_cast<float>(m.d1), static_cast<float>(m.d2), static_cast<float>(m.d3), static_cast<float>(m.d4));
        return out;
    }

    // Mat4 -> aiMatrix4x4
    inline aiMatrix4x4 Mat4ToAiMatrix4x4(const Mat4 &m) noexcept
    {
        aiMatrix4x4 out;
        out.a1 = static_cast<float>(m.rows[0].x);
        out.a2 = static_cast<float>(m.rows[0].y);
        out.a3 = static_cast<float>(m.rows[0].z);
        out.a4 = static_cast<float>(m.rows[0].w);

        out.b1 = static_cast<float>(m.rows[1].x);
        out.b2 = static_cast<float>(m.rows[1].y);
        out.b3 = static_cast<float>(m.rows[1].z);
        out.b4 = static_cast<float>(m.rows[1].w);

        out.c1 = static_cast<float>(m.rows[2].x);
        out.c2 = static_cast<float>(m.rows[2].y);
        out.c3 = static_cast<float>(m.rows[2].z);
        out.c4 = static_cast<float>(m.rows[2].w);

        out.d1 = static_cast<float>(m.rows[3].x);
        out.d2 = static_cast<float>(m.rows[3].y);
        out.d3 = static_cast<float>(m.rows[3].z);
        out.d4 = static_cast<float>(m.rows[3].w);

        return out;
    }

    // helper: recursive traversal to create instances for each node->mesh reference ---
    static void TraverseAssimpNodesAndCretaeInstances(const aiScene* scene, const aiNode* node, const aiMatrix4x4& parentTransform, World* world, const std::vector<int>& aiToWorldMesh, const std::vector<int>& aiToWorldMat)
    {
        if (!node)
        {
            return;
        }

        aiMatrix4x4 global = parentTransform * node->mTransformation;

        // for each mesh referenced by this node, create an instance with the global transform
        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            uint32_t aiMeshIndex = node->mMeshes[i];
            if (aiMeshIndex >= aiToWorldMesh.size())
            {
                continue;
            }
            int worldMeshIndex = aiToWorldMesh[aiMeshIndex];
            if (worldMeshIndex < 0 || worldMeshIndex >= static_cast<int>(world->m_meshes.size()))
            {
                continue;
            }

            // get material id from the original aiMesh
            int worldMat = 0;
            const aiMesh *am = scene->mMeshes[aiMeshIndex];
            if (am && am->mMaterialIndex >= 0 && am->mMaterialIndex < static_cast<int>(aiToWorldMat.size()))
            {
                worldMat = aiToWorldMat[am->mMaterialIndex];
            }

            std::string instName = world->m_meshes[worldMeshIndex]->name + "_instance";
            Mat4 xform = AiToMat4(global);

            // create the instance
            world->m_meshInstances.emplace_back(instName, xform, worldMeshIndex, worldMat);
        }

        // recurse children
        for (uint32_t c = 0; c < node->mNumChildren; ++c)
        {
            TraverseAssimpNodesAndCretaeInstances(scene, node->mChildren[c], global, world, aiToWorldMesh, aiToWorldMat);
        }
    }

    static std::string SaveEmbeddedTextureToTemFile(const aiTexture* atex, const std::filesystem::path& basePath, int embIndex)
    { 
        if (!atex)
        {
            return "";
        }

        // compressed (mHeight == 0) stores pcData with size = mWidth
        if (atex->mHeight == 0 && atex->pcData)
        {
            //std::string fmt = atex->achFormatHint ? std::string(atex->achFormatHint) : "png";
            //// normalize format hint (some are like "PNG\0")
            //for (auto& c : fmt)
            //{
            //    if (c == '\0')
            //    {
            //        fmt.resize(&c - fmt.c_str());
            //        break;
            //    }
            //}
            std::string fmt = "";
            if (atex->achFormatHint && atex->achFormatHint[0] != '\0')
            {
                fmt = std::string(atex->achFormatHint);
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
            std::filesystem::path outPath = basePath / name;

            std::ofstream ofs(outPath, std::ios::binary);
            if (!ofs)
            {
                return "";
            }
            ofs.write(reinterpret_cast<const char *>(atex->pcData), static_cast<std::streamsize>(atex->mWidth));
            ofs.close();
            return outPath.string();
        }

        return "";
    }

    // helper: robustly try several textures slots and return world texture id (or -1).
    static int ResolveAndLoadTextureFromMaterial(aiMaterial* aim, const aiScene* scene, World* world, DeviceContext* device, const std::filesystem::path& basepath, const std::vector<aiTextureType>& tryTypes)
    {
        aiString texPath;
        // try each texture type in order
        for (auto type : tryTypes)
        {
            if (aim->GetTexture(type, 0, &texPath) == AI_SUCCESS)
            {
                std::string p = texPath.C_Str();
                if (p.empty())
                {
                    continue;
                }

                //embedded texture reference link "*0"
                if (p.front() == '*')
                {
                    // parse index
                    int emb = 0;
                    try
                    {
                        emb = std::stoi(p.substr(1));
                    }
                    catch (...)
                    {
                        continue;
                    }
                    if (emb >= 0 && emb < static_cast<int>(scene->mNumTextures))
                    {
                        aiTexture *atex = scene->mTextures[emb];
                        std::string tmp = SaveEmbeddedTextureToTemFile(atex, basepath, emb);
                        if (!tmp.empty())
                        {
                            return world->EnsureTextureCached(device, tmp);
                        }
                        // todo: else embedding is raw RGBA
                    }
                }
                else
                {
                    // external path: make absolute relative to basePath if not absolute
                    std::filesystem::path ppath(p);
                    std::filesystem::path resolved;
                    if (ppath.is_absolute())
                    {
                        resolved = ppath;
                    }
                    else
                    {
                        resolved = (basepath / ppath).lexically_normal();
                    }

                    return world->EnsureTextureCached(device, resolved.string());
                }
            }
        }
        return -1;
    }

    static Material ConvertAssimpMaterial(aiMaterial* aim, const aiScene* scene, World* world, DeviceContext* device, const std::filesystem::path& basePath)
    {
        Material mat; // default ctor sets defaults

        // name
        aiString name;
        if (aim->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
        {
            mat.name = name.C_Str();
        }

        // base color: many exporters use COLOR_DIFFUSE; gltf PBR uses PBR factor keys
        aiColor4D diffuse(1.f, 1.f, 1.f, 1.f);
        if (aim->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS)
        {
            mat.baseColor = Vec3(static_cast<float>(diffuse.r), static_cast<float>(diffuse.g), static_cast<float>(diffuse.b));
            mat.opacity = static_cast<float>(diffuse.a);
        }

        float opacity = 1.f;
        if (aim->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
        {
            mat.opacity = opacity;
        }

        // glTF pbr factors
        float metal = 0.f, rough = 1.f;
        aim->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, metal);
        aim->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, rough);
        mat.metallic = metal;
        mat.roughness = sqrtf((rough > 0.f) ? rough : 0.f);

        // emissive 
        aiColor3D emissive(0.f, 0.f, 0.f);
        if (aim->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
        {
            mat.emission = Vec3(static_cast<float>(emissive.r), static_cast<float>(emissive.g), static_cast<float>(emissive.b));
        }

        // alpha cutoff
        float alphaCutoff = 0.f;
        if (aim->Get(AI_MATKEY_GLTF_ALPHACUTOFF, alphaCutoff) == AI_SUCCESS)
        {
            mat.alphaCutoff = alphaCutoff;
        }

        // alphaMode: try to read property; Assimp may keep "alphaMode" string as a property name
        // default keep Opaque; we'll try to find a property that contains "alpha"
        mat.alphaMode = AlphaMode::Opaque;
        for (uint32_t p = 0; p < aim->mNumProperties; ++p)
        {
            aiMaterialProperty *prop = aim->mProperties[p];
            if (!prop || !prop->mKey.length)
            {
                continue;
            }
            std::string key = prop->mKey.C_Str();
            std::string kl = ToLowerStr(key);
            if (kl.find("alphamode") != std::string::npos || kl.find("alpha_mode") != std::string::npos)
            {
                // data is stored as string
                std::string val(reinterpret_cast<const char*>(prop->mData), prop->mDataLength);
                // trim null/newline
                while (!val.empty() && (val.back() == '\0' || std::isspace((unsigned char)val.back())))
                {
                    val.pop_back();
                }
                val = ToLowerStr(val);
                if (val.find("blend") != std::string::npos)
                {
                    mat.alphaMode = AlphaMode::Blend;
                }
                else if (val.find("mask") != std::string::npos)
                {
                    mat.alphaMode = AlphaMode::Mask;
                }
                else
                {
                    mat.alphaMode = AlphaMode::Opaque;
                }
                break;
            }
        }

        // Texture Loading try candidate slots for each semantic
        // baseColor: try base color(if supported) DiFFUSE UNKNOWN
        {
            std::vector<aiTextureType> trySlots = {aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR};
            int tid = ResolveAndLoadTextureFromMaterial(aim, scene, world, device, basePath, trySlots);
            if (tid >= 0)
            {
                mat.baseColorTexId = tid;
            }
        }

        // normal map
        {
            std::vector<aiTextureType> trySlots = {aiTextureType_NORMALS, aiTextureType_HEIGHT};
            int tid = ResolveAndLoadTextureFromMaterial(aim, scene, world, device, basePath, trySlots);
            if (tid >= 0)
            {
                mat.normalMapTexId = tid;
            }
        }

        // metallic roughness map
        {
            std::vector<aiTextureType> trySlots = {aiTextureType_GLTF_METALLIC_ROUGHNESS, aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS};
            int tid = ResolveAndLoadTextureFromMaterial(aim, scene, world, device, basePath, trySlots);
            if (tid >= 0)
            {
                mat.metallicRoughtnessTexId = tid;
            }
        }

        // emissive map
        {
            std::vector<aiTextureType> trySlots = {aiTextureType_EMISSIVE};
            int tid = ResolveAndLoadTextureFromMaterial(aim, scene, world, device, basePath, trySlots);
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
            else if (mat.alphaCutoff < 0.999f)
            {
                mat.alphaMode = AlphaMode::Mask;
            }
        }

        return mat;
    }

	int World::LoadMeshGeometryOnly(DeviceContext *device, const std::string &filename)
    {
        std::string key = NormalizePathForKey(filename);
        auto it = m_meshIndexByKey.find(key);
        if (it != m_meshIndexByKey.end())
        {
            return it->second;
        }

        Assimp::Importer importer;
        unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality;

        const aiScene* scene = importer.ReadFile(filename, flags);
        if(!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !scene->mRootNode)
        {
            std::cerr << "Assimp: failed to load " << filename << ": " << importer.GetErrorString() << "\n";
            return -1;
        }

        ElecNekoMesh* mesh = new ElecNekoMesh();
        mesh->name = std::filesystem::path(filename).stem().string();

        // pre-alloc heuristics 
        size_t totalVerticesHint = 0;
        size_t totalFacesHint = 0;

        for (uint32_t mi = 0; mi < scene->mNumMeshes; mi++)
        {
            totalVerticesHint += scene->mMeshes[mi]->mNumVertices;
            totalFacesHint += scene->mMeshes[mi]->mNumFaces;
        }
        mesh->m_vertices.reserve(totalVerticesHint);
        mesh->m_indices.reserve(totalFacesHint * 3);

        std::unordered_map<VVertex, uint32_t, VVertexHash> dedup;
        dedup.reserve((totalVerticesHint > 0) ? static_cast<size_t>(totalVerticesHint * 2) : 128);

        // iterate each aiMesh and accumulate triangles
        for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
        {
            aiMesh *am = scene->mMeshes[mi];
            if(!am) continue;

            // build index/vertex buffers (dedupe)
            for (unsigned int fi = 0; fi < am->mNumFaces; ++fi)
            {
                const aiFace &face = am->mFaces[fi];
                if(face.mNumIndices != 3) continue;

                for (unsigned int k = 0; k < 3; ++k)
                {
                    uint32_t idx = face.mIndices[k];
                    VVertex vv{};
                    if(am->HasPositions())
                    {
                        vv.position[0] = am->mVertices[idx].x;
                        vv.position[1] = am->mVertices[idx].y;
                        vv.position[2] = am->mVertices[idx].z;
                    }
                    else
                    {
                        vv.position[0] = vv.position[1] = vv.position[2] = 0.f;
                    }
                    
                    if(am->HasTextureCoords(0))
                    {
                        vv.uv[0] = am->mTextureCoords[0][idx].x;
                        vv.uv[1] = 1.f - am->mTextureCoords[0][idx].y;
                    }
                    else
                    {
                        vv.uv[0] = vv.uv[1] = 0.f;
                    }
                    
                    if(am->HasNormals())
                    {
                        vv.normal[0] = am->mNormals[idx].x;
                        vv.normal[1] = am->mNormals[idx].y;
                        vv.normal[2] = am->mNormals[idx].z;
                    }
                    else
                    {
                        vv.normal[0] = 0.f;
                        vv.normal[1] = 0.f;
                        vv.normal[2] = 1.f;
                    }

                    auto it = dedup.find(vv);
                    if(it == dedup.end())
                    {
                        uint32_t newIdx = static_cast<uint32_t>(mesh->m_vertices.size());
                        mesh->m_vertices.push_back(vv);
                        mesh->m_indices.push_back(newIdx);
                        dedup.emplace(vv, newIdx);
                    }
                    else
                    {
                        mesh->m_indices.push_back(it->second);
                    }
                }
            }
        }

        if(mesh->m_vertices.empty() || mesh->m_indices.empty())
        {
            delete mesh;
            return -1;
        }

        if(!mesh->MakeVBO(device))
        {
            std::cerr << "Failed to make VBO\n";
            delete mesh;
            return -1;
        }

        int firstIndex = static_cast<int>(m_meshes.size());
        m_meshes.push_back(mesh);
        m_meshIndexByKey.emplace(key, firstIndex);
        return firstIndex;
    }

    int World::EnsureTextureCached(DeviceContext *device, const std::string &filename)
    {
        std::string key = NormalizePathForKey(filename);

        const auto& it = m_textureCache.find(key);
        if(it != m_textureCache.end())
        {
            return it->second;
        }

        int texIndex = AddTexture(device, filename);
        if (texIndex >= 0)
        {
            m_textureCache.emplace(key, texIndex);
        }
        return texIndex;
    }

    // Main function: load geometry + materials. For each aiMesh we create one ElecNekoMesh and return per-part material ids.
    int World::LoadMeshWithMaterials(DeviceContext *device, const std::string &filename, Mat4 transMat)
    {
        Assimp::Importer importer;
        unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality;

        const aiScene* scene = importer.ReadFile(filename, flags);
        if(!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !scene->mRootNode)
        {
            std::cerr << "Assimp: failed to load " << filename << ": " << importer.GetErrorString() << "\n";
            return -1;
        }

        std::filesystem::path basePath = std::filesystem::path(filename).parent_path();
        int firstIndex = static_cast<int>(m_meshes.size());
        
        // 1) convert materials -> World::m_materials and keep ai index -> world index mapping
        std::vector<int> aiToWorldMat(scene->mNumMaterials, -1);
        
        for (uint32_t aiMatIdx = 0; aiMatIdx < scene->mNumMaterials; ++aiMatIdx)
        {
            aiMaterial *aim = scene->mMaterials[aiMatIdx];
            Material mat = ConvertAssimpMaterial(aim, scene, this, device, basePath);
            int wid = AddMaterial(mat);
            aiToWorldMat[aiMatIdx] = wid;
        }

        if (m_materials.empty())
        {
            Material def;
            AddMaterial(def);
        }

        // 2) create ElecNekoMesh for each aiMesh and record the world material id per created mesh
        std::vector<int> aiToWorldMesh(scene->mNumMeshes, -1);

        for (uint32_t mi = 0; mi < scene->mNumMeshes; ++mi)
        {
            aiMesh *am = scene->mMeshes[mi];
            if (!am)
            {
                continue;
            }

            ElecNekoMesh *mesh = new ElecNekoMesh();
            mesh->name = (am->mName.C_Str() && am->mName.length) ? am->mName.C_Str()
                                                                 : (std::filesystem::path(filename).stem().string() + "_part" + std::to_string(mi));


            // reserve
            mesh->m_vertices.reserve(am->mNumVertices);
            mesh->m_indices.reserve(am->mNumFaces * 3);

            std::unordered_map<VVertex, uint32_t, VVertexHash> dedup;
            dedup.reserve(am->mNumVertices * 2);

            for (uint32_t fi = 0; fi < am->mNumFaces; ++fi)
            {
                const aiFace &face = am->mFaces[fi];
                if (face.mNumIndices != 3)
                {
                    continue;
                }

                for (uint32_t k = 0; k < 3; k++)
                {
                    uint32_t idx = face.mIndices[k];

                    VVertex vv{};
                    if (am->HasPositions())
                    {
                        vv.position[0] = am->mVertices[idx].x;
                        vv.position[1] = am->mVertices[idx].y;
                        vv.position[2] = am->mVertices[idx].z;
                    }
                    else
                    {
                        vv.position[0] = vv.position[1] = vv.position[2] = 0.f;
                    }

                    if (am->HasTextureCoords(0))
                    {
                        vv.uv[0] = am->mTextureCoords[0][idx].x;
                        vv.uv[1] = 1.0f - am->mTextureCoords[0][idx].y;
                    }
                    else
                    {
                        vv.uv[0] = vv.uv[1] = 0.f;
                    }

                    if (am->HasNormals())
                    {
                        vv.normal[0] = am->mNormals[idx].x;
                        vv.normal[1] = am->mNormals[idx].y;
                        vv.normal[2] = am->mNormals[idx].z;
                    }
                    else
                    {
                        vv.normal[0] = 0.f;
                        vv.normal[1] = 0.f;
                        vv.normal[2] = 1.f;
                    }

                    auto it = dedup.find(vv);
                    if (it == dedup.end())
                    {
                        uint32_t newIdx = static_cast<uint32_t>(mesh->m_vertices.size());
                        mesh->m_vertices.push_back(vv);
                        mesh->m_indices.push_back(newIdx);
                        dedup.emplace(vv, newIdx);
                    }
                    else
                    {
                        mesh->m_indices.push_back(it->second);
                    }
                }
            }

            // upload vbo
            if (!mesh->MakeVBO(device))
            {
                std::cerr << "Warning: MakeVBO failed for mesh " << mesh->name << "\n";
                delete mesh;
                continue;
            }

            int worldMeshIndex = static_cast<int>(m_meshes.size());
            m_meshes.push_back(mesh);
            aiToWorldMesh[mi] = worldMeshIndex;
        }

        aiMatrix4x4 trans = Mat4ToAiMatrix4x4(transMat);
        TraverseAssimpNodesAndCretaeInstances(scene, scene->mRootNode, trans, this, aiToWorldMesh, aiToWorldMat);

        if (scene->mNumMeshes == 0)
            return -1;

        return firstIndex;
    }

    void World::CreateDefaultTextures(DeviceContext* device)
    { 
        std::array<uint8_t, 4> white = {255, 255, 255, 255};
        defaultAlbedo = new Texture(device, "default_albedo", white.data(), 1, 1, 4);

        std::array<uint8_t, 4> normal = {128, 128, 255, 255};
        defaultNormal = new Texture(device, "default_normal", normal.data(), 1, 1, 4);

        std::array<uint8_t, 4> metalRough = {0, 128, 255, 255};
        defaultMetalRough = new Texture(device, "default_metallic_roughness", metalRough.data(), 1, 1, 4);

        std::array<uint8_t, 4> black = {0, 0, 0, 255};
        defaultEmission = new Texture(device, "default_emission", black.data(), 1, 1, 4);
    }

    int World::AddTexture(DeviceContext* device, const std::string& filename) 
    { 
        Texture *texture = new Texture();

        if (!texture->LoadTexture(device, filename))
        {
            std::cerr << "Failed to load texture " + filename << "\n";
            delete texture;
            return -1;
        }

        int idx = m_textures.size();
        m_textures.push_back(texture);
        return idx;
    }

    int World::AddMaterial(const Material& material)
    { 
        int idx = m_materials.size();
        m_materials.push_back(material);
        return idx;
    }

    void World::AddCamera(Vec3 eye, Vec3 lookAt, float fov, float aspecRatio, float zNear, float zFar)
    { 
        if (m_cam)
            delete m_cam;

        m_cam = new Camera;
        m_cam->Initialize(eye, lookAt, fov, aspecRatio, zNear, zFar);
    }

    bool World::LoadSceneFromFile(DeviceContext* device, const std::string& filename) 
    {
        std::ifstream ifs(filename);
        if (!ifs.is_open())
        {
            std::cerr << "Filed to open scene file: " << filename << "\n";
            return false;
        }

        std::filesystem::path basePath = std::filesystem::path(filename).parent_path();

        // material name ->world material id
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
                for (auto& ln : block)
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
                    else if (ln.rfind("metallicroughnesstexture", 0) == 0)
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
                }

                mat.name = materialName;
                if (materialMap.find(materialName) == materialMap.end())
                {
                    int matId = AddMaterial(mat);
                    materialMap.emplace(materialName, matId);
                }
                continue;
            }
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
                    int meshIdx = LoadMeshGeometryOnly(device, full);
                    if (meshIdx >= 0)
                    {
                        int materialId = 0;
                        if (!matName.empty() && materialMap.count(matName))
                        {
                            materialId = materialMap[matName];
                        }
                        Mat4 transform;
                        transform = scale * rotation * translate;
                        // instance
                        std::string instanceName = (meshName.empty()) ? std::filesystem::path(fileTok).stem().string() : meshName;
                        instanceName += "_instance";
                        m_meshInstances.emplace_back(instanceName, transform, meshIdx, materialId);
                    }
                }
                continue;
            }
            if (token == "gltf")
            {
                auto block = ReadBlock(ifs, t);
                std::string fileTok;
                Mat4 translate, scale, rotation;
                translate.Identity();
                scale.Identity();
                rotation.Identity();
                for (auto& ln : block)
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
                    Mat4 transform;
                    transform = scale * rotation * translate;
                    LoadMeshWithMaterials(device, full, transform);
                }
                continue;
            }
            if (token == "camera")
            {
                auto block = ReadBlock(ifs, t);
                Vec3 pos;
                Vec3 lookAt;
                float fov = 45.f;
                float aperture = 0.f;
                float focalDist = 1.f;

                for (auto& ln : block)
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
                m_cam->aperture = aperture;
                m_cam->focalDist = focalDist;
                continue;
            }
        }
        return true;
    }

    void World::UnloadScene(DeviceContext* device)
    { 
        for (auto& inst : m_meshInstances)
        {
            inst.Cleanup(device);
        }
        m_meshInstances.clear();

        for (auto* mesh : m_meshes)
        {
            if (mesh)
            {
                mesh->Cleanup(device);
                delete mesh;
            }
        }
        m_meshes.clear();
        m_meshIndexByKey.clear();

        for (auto* tex : m_textures)
        {
            if (tex)
            {
                tex->Cleanup(device);
                delete tex;
            }
        }
        m_textures.clear();
        m_textureCache.clear();

        m_materials.clear();

        if (m_cam)
        {
            delete m_cam;
            m_cam = nullptr;
        }
    }

    void World::Cleanup(DeviceContext* device)
    { 
        UnloadScene(device);
        
        defaultAlbedo->Cleanup(device);
        defaultNormal->Cleanup(device);
        defaultMetalRough->Cleanup(device);
        defaultEmission->Cleanup(device);

        delete defaultAlbedo;
        delete defaultNormal;
        delete defaultMetalRough;
        delete defaultEmission;
    }
}
