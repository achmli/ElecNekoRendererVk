// src/Renderer/Scene/SceneFileLoader.cpp
#include "Renderer/Scene/SceneFileLoader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace ElecNeko
{
    namespace
    {
        static std::string Trim(const std::string &s)
        {
            size_t begin = 0;
            while (begin < s.size() && std::isspace(static_cast<unsigned char>(s[begin])))
            {
                ++begin;
            }

            size_t end = s.size();
            while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1])))
            {
                --end;
            }

            return s.substr(begin, end - begin);
        }

        static std::string ToLower(std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            return s;
        }

        static bool IsCommentOrEmpty(const std::string &line)
        {
            if (line.empty())
            {
                return true;
            }

            if (line[0] == '#')
            {
                return true;
            }

            if (line.size() >= 2 && line[0] == '/' && line[1] == '/')
            {
                return true;
            }

            return false;
        }

        static std::vector<std::string> Tokenize(const std::string &line)
        {
            std::vector<std::string> tokens;

            std::istringstream iss(line);
            std::string token;

            while (iss >> token)
            {
                tokens.push_back(token);
            }

            return tokens;
        }

        static bool ReadVec3(const std::vector<std::string> &tokens, Vec3 &out)
        {
            if (tokens.size() < 4)
            {
                return false;
            }

            out = Vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));

            return true;
        }

        static bool ReadQuat(const std::vector<std::string> &tokens, SceneQuatDesc &out)
        {
            if (tokens.size() < 5)
            {
                return false;
            }

            out.x = std::stof(tokens[1]);
            out.y = std::stof(tokens[2]);
            out.z = std::stof(tokens[3]);
            out.w = std::stof(tokens[4]);

            return true;
        }

        static bool ReadFloat(const std::vector<std::string> &tokens, float &out)
        {
            if (tokens.size() < 2)
            {
                return false;
            }

            out = std::stof(tokens[1]);
            return true;
        }

        static bool ReadInt2(const std::vector<std::string> &tokens, int &x, int &y)
        {
            if (tokens.size() < 3)
            {
                return false;
            }

            x = std::stoi(tokens[1]);
            y = std::stoi(tokens[2]);
            return true;
        }

        static SceneAssetFormat FormatFromBlockName(const std::string &blockName)
        {
            const std::string lower = ToLower(blockName);

            if (lower == "obj")
            {
                return SceneAssetFormat::Obj;
            }

            if (lower == "gltf" || lower == "glb")
            {
                return SceneAssetFormat::Gltf;
            }

            if (lower == "fbx")
            {
                return SceneAssetFormat::Fbx;
            }

            return SceneAssetFormat::Other;
        }

        static SceneAlphaMode ParseAlphaMode(const std::string &s)
        {
            const std::string lower = ToLower(s);

            if (lower == "mask" || lower == "masked" || lower == "alpha_test" || lower == "alphatest")
            {
                return SceneAlphaMode::Mask;
            }

            if (lower == "blend" || lower == "transparent" || lower == "alpha_blend")
            {
                return SceneAlphaMode::Blend;
            }

            return SceneAlphaMode::Opaque;
        }

        static SceneMediumType ParseMediumType(const std::string &s)
        {
            const std::string lower = ToLower(s);

            if (lower == "absorb" || lower == "absorption")
            {
                return SceneMediumType::Absorb;
            }

            if (lower == "scatter" || lower == "scattering")
            {
                return SceneMediumType::Scatter;
            }

            return SceneMediumType::None;
        }

        static SceneLightType ParseLightType(const std::string &s)
        {
            const std::string lower = ToLower(s);

            if (lower == "quad")
            {
                return SceneLightType::Quad;
            }

            if (lower == "sphere")
            {
                return SceneLightType::Sphere;
            }

            if (lower == "directional" || lower == "dir")
            {
                return SceneLightType::Directional;
            }

            if (lower == "point")
            {
                return SceneLightType::Point;
            }

            return SceneLightType::Unknown;
        }

        static void ParseTransformField(const std::vector<std::string> &tokens, SceneTransformDesc &transform)
        {
            if (tokens.empty())
            {
                return;
            }

            const std::string key = ToLower(tokens[0]);

            if (key == "position" || key == "translate" || key == "translation")
            {
                if (ReadVec3(tokens, transform.position))
                {
                    transform.hasPosition = true;
                }
            }
            else if (key == "scale")
            {
                if (tokens.size() == 2)
                {
                    const float s = std::stof(tokens[1]);
                    transform.scale = Vec3(s, s, s);
                    transform.hasScale = true;
                }
                else if (tokens.size() >= 4)
                {
                    transform.scale = Vec3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
                    transform.hasScale = true;
                }
            }
            else if (key == "rotation" || key == "rotate")
            {
                if (ReadQuat(tokens, transform.rotation))
                {
                    transform.hasRotation = true;
                }
            }
        }

        class SceneParser
        {
        public:
            explicit SceneParser(const std::filesystem::path &sceneFile) : m_sceneFile(sceneFile) {}

            bool Parse(SceneLoadDesc &outDesc)
            {
                std::ifstream file(m_sceneFile);

                if (!file.is_open())
                {
                    std::cerr << "Failed to open scene file: " << m_sceneFile << "\n";
                    return false;
                }

                outDesc = SceneLoadDesc{};
                outDesc.sceneFile = m_sceneFile;
                outDesc.baseDirectory = m_sceneFile.parent_path();

                std::string line;

                while (std::getline(file, line))
                {
                    line = StripInlineComment(Trim(line));

                    if (IsCommentOrEmpty(line))
                    {
                        continue;
                    }

                    std::vector<std::string> tokens = Tokenize(line);

                    if (tokens.empty())
                    {
                        continue;
                    }

                    std::string blockName = ToLower(tokens[0]);

                    if (blockName == "{")
                    {
                        continue;
                    }

                    if (blockName == "renderer")
                    {
                        ParseRendererBlock(file, outDesc.renderer);
                    }
                    else if (blockName == "camera")
                    {
                        ParseCameraBlock(file, outDesc.camera);
                    }
                    else if (blockName == "material")
                    {
                        SceneMaterialDesc material{};

                        if (tokens.size() >= 2)
                        {
                            material.name = tokens[1];
                        }

                        ParseMaterialBlock(file, material);
                        outDesc.materials.push_back(material);
                    }
                    else if (blockName == "mesh")
                    {
                        SceneMeshInstanceDesc mesh{};
                        ParseMeshBlock(file, mesh);
                        outDesc.meshInstances.push_back(mesh);
                    }
                    else if (blockName == "obj" || blockName == "gltf" || blockName == "glb" || blockName == "fbx")
                    {
                        SceneModelInstanceDesc model{};
                        model.format = FormatFromBlockName(blockName);
                        ParseModelBlock(file, model);
                        outDesc.modelInstances.push_back(model);
                    }
                    else if (blockName == "light")
                    {
                        SceneLightDesc light{};

                        if (tokens.size() >= 2)
                        {
                            light.name = tokens[1];
                        }

                        ParseLightBlock(file, light);
                        outDesc.lights.push_back(light);
                    }
                    else
                    {
                    }
                }

                return true;
            }

        private:
            static std::string StripInlineComment(const std::string &line)
            {
                size_t hash = line.find('#');
                size_t slash = line.find("//");

                size_t pos = std::string::npos;

                if (hash != std::string::npos)
                {
                    pos = hash;
                }

                if (slash != std::string::npos)
                {
                    pos = std::min(pos, slash);
                }

                if (pos != std::string::npos)
                {
                    return Trim(line.substr(0, pos));
                }

                return line;
            }

            static bool IsBlockEnd(const std::string &line) { return Trim(line) == "}"; }

            static void ConsumeOpeningBraceIfNeeded(std::ifstream &file)
            {
                std::streampos oldPos = file.tellg();

                std::string line;
                if (!std::getline(file, line))
                {
                    return;
                }

                line = StripInlineComment(Trim(line));

                if (line != "{")
                {
                    file.seekg(oldPos);
                }
            }

            void ParseRendererBlock(std::ifstream &file, SceneRendererDesc &renderer)
            {
                ConsumeOpeningBraceIfNeeded(file);

                std::string line;

                while (std::getline(file, line))
                {
                    line = StripInlineComment(Trim(line));

                    if (IsCommentOrEmpty(line))
                    {
                        continue;
                    }

                    if (IsBlockEnd(line))
                    {
                        break;
                    }

                    std::vector<std::string> tokens = Tokenize(line);

                    if (tokens.empty())
                    {
                        continue;
                    }

                    const std::string key = ToLower(tokens[0]);

                    if (key == "resolution")
                    {
                        ReadInt2(tokens, renderer.resolutionX, renderer.resolutionY);
                    }
                    else if (key == "windowresolution")
                    {
                        ReadInt2(tokens, renderer.windowResolutionX, renderer.windowResolutionY);
                    }
                    else if (key == "maxdepth")
                    {
                        if (tokens.size() >= 2)
                        {
                            renderer.maxDepth = std::stoi(tokens[1]);
                        }
                    }
                    else if (key == "tilewidth")
                    {
                        if (tokens.size() >= 2)
                        {
                            renderer.tileWidth = std::stoi(tokens[1]);
                        }
                    }
                    else if (key == "tileheight")
                    {
                        if (tokens.size() >= 2)
                        {
                            renderer.tileHeight = std::stoi(tokens[1]);
                        }
                    }
                    else if (key == "envmapfile")
                    {
                        if (tokens.size() >= 2)
                        {
                            renderer.envMapFile = tokens[1];
                        }
                    }
                    else if (key == "envmapintensity")
                    {
                        ReadFloat(tokens, renderer.envMapIntensity);
                    }
                    else if (key == "independentrendersize")
                    {
                        if (tokens.size() >= 2)
                        {
                            renderer.independentRenderSize = ToLower(tokens[1]) == "true" || tokens[1] == "1";
                        }
                    }
                }
            }

            void ParseCameraBlock(std::ifstream &file, SceneCameraDesc &camera)
            {
                ConsumeOpeningBraceIfNeeded(file);

                std::string line;

                while (std::getline(file, line))
                {
                    line = StripInlineComment(Trim(line));

                    if (IsCommentOrEmpty(line))
                    {
                        continue;
                    }

                    if (IsBlockEnd(line))
                    {
                        break;
                    }

                    std::vector<std::string> tokens = Tokenize(line);

                    if (tokens.empty())
                    {
                        continue;
                    }

                    const std::string key = ToLower(tokens[0]);

                    if (key == "position")
                    {
                        ReadVec3(tokens, camera.position);
                        camera.valid = true;
                    }
                    else if (key == "lookat")
                    {
                        ReadVec3(tokens, camera.lookAt);
                        camera.valid = true;
                    }
                    else if (key == "fov")
                    {
                        ReadFloat(tokens, camera.fov);
                        camera.valid = true;
                    }
                }
            }

            void ParseMaterialBlock(std::ifstream &file, SceneMaterialDesc &material)
            {
                ConsumeOpeningBraceIfNeeded(file);

                std::string line;

                while (std::getline(file, line))
                {
                    line = StripInlineComment(Trim(line));

                    if (IsCommentOrEmpty(line))
                    {
                        continue;
                    }

                    if (IsBlockEnd(line))
                    {
                        break;
                    }

                    std::vector<std::string> tokens = Tokenize(line);

                    if (tokens.empty())
                    {
                        continue;
                    }

                    const std::string key = ToLower(tokens[0]);

                    if (key == "color" || key == "basecolor" || key == "albedo")
                    {
                        ReadVec3(tokens, material.baseColor);
                    }
                    else if (key == "opacity")
                    {
                        ReadFloat(tokens, material.opacity);
                    }
                    else if (key == "emission")
                    {
                        ReadVec3(tokens, material.emission);
                    }
                    else if (key == "metallic" || key == "metalness")
                    {
                        ReadFloat(tokens, material.metallic);
                    }
                    else if (key == "roughness")
                    {
                        ReadFloat(tokens, material.roughness);
                    }
                    else if (key == "spectrans")
                    {
                        ReadFloat(tokens, material.specTrans);
                    }
                    else if (key == "speculartint")
                    {
                        ReadFloat(tokens, material.specularTint);
                    }
                    else if (key == "anisotropic")
                    {
                        ReadFloat(tokens, material.anisotropic);
                    }
                    else if (key == "subsurface")
                    {
                        ReadFloat(tokens, material.subsurface);
                    }
                    else if (key == "sheen")
                    {
                        ReadFloat(tokens, material.sheen);
                    }
                    else if (key == "sheentint")
                    {
                        ReadFloat(tokens, material.sheenTint);
                    }
                    else if (key == "clearcoat")
                    {
                        ReadFloat(tokens, material.clearcoat);
                    }
                    else if (key == "clearcoatgloss")
                    {
                        ReadFloat(tokens, material.clearcoatGloss);
                    }
                    else if (key == "ior")
                    {
                        ReadFloat(tokens, material.ior);
                    }
                    else if (key == "mediumtype")
                    {
                        if (tokens.size() >= 2)
                        {
                            material.mediumType = ParseMediumType(tokens[1]);
                        }
                    }
                    else if (key == "mediumdensity")
                    {
                        ReadFloat(tokens, material.mediumDensity);
                    }
                    else if (key == "mediumcolor")
                    {
                        ReadVec3(tokens, material.mediumColor);
                    }
                    else if (key == "mediumanisotropy")
                    {
                        ReadFloat(tokens, material.mediumAnisotropy);
                    }
                    else if (key == "alphamode")
                    {
                        if (tokens.size() >= 2)
                        {
                            material.alphaMode = ParseAlphaMode(tokens[1]);
                        }
                    }
                    else if (key == "alphacutoff")
                    {
                        ReadFloat(tokens, material.alphaCutoff);
                    }
                    else if (key == "albedotexture" || key == "basecolortexture" || key == "colortexture")
                    {
                        if (tokens.size() >= 2)
                        {
                            material.baseColorTexture = tokens[1];
                        }
                    }
                    else if (key == "normaltexture")
                    {
                        if (tokens.size() >= 2)
                        {
                            material.normalTexture = tokens[1];
                        }
                    }
                    else if (key == "metalroughtexture" || key == "metallicroughnesstexture")
                    {
                        if (tokens.size() >= 2)
                        {
                            material.metalRoughTexture = tokens[1];
                        }
                    }
                    else if (key == "emissiontexture")
                    {
                        if (tokens.size() >= 2)
                        {
                            material.emissionTexture = tokens[1];
                        }
                    }
                }
            }

            void ParseMeshBlock(std::ifstream &file, SceneMeshInstanceDesc &mesh)
            {
                ConsumeOpeningBraceIfNeeded(file);

                std::string line;

                while (std::getline(file, line))
                {
                    line = StripInlineComment(Trim(line));

                    if (IsCommentOrEmpty(line))
                    {
                        continue;
                    }

                    if (IsBlockEnd(line))
                    {
                        break;
                    }

                    std::vector<std::string> tokens = Tokenize(line);

                    if (tokens.empty())
                    {
                        continue;
                    }

                    const std::string key = ToLower(tokens[0]);

                    if (key == "name")
                    {
                        if (tokens.size() >= 2)
                        {
                            mesh.name = tokens[1];
                        }
                    }
                    else if (key == "file")
                    {
                        if (tokens.size() >= 2)
                        {
                            mesh.file = tokens[1];
                        }
                    }
                    else if (key == "material")
                    {
                        if (tokens.size() >= 2)
                        {
                            mesh.materialName = tokens[1];
                        }
                    }
                    else
                    {
                        ParseTransformField(tokens, mesh.transform);
                    }
                }

                if (mesh.name.empty())
                {
                    mesh.name = mesh.file;
                }
            }

            void ParseModelBlock(std::ifstream &file, SceneModelInstanceDesc &model)
            {
                ConsumeOpeningBraceIfNeeded(file);

                std::string line;

                while (std::getline(file, line))
                {
                    line = StripInlineComment(Trim(line));

                    if (IsCommentOrEmpty(line))
                    {
                        continue;
                    }

                    if (IsBlockEnd(line))
                    {
                        break;
                    }

                    std::vector<std::string> tokens = Tokenize(line);

                    if (tokens.empty())
                    {
                        continue;
                    }

                    const std::string key = ToLower(tokens[0]);

                    if (key == "name")
                    {
                        if (tokens.size() >= 2)
                        {
                            model.name = tokens[1];
                        }
                    }
                    else if (key == "file")
                    {
                        if (tokens.size() >= 2)
                        {
                            model.file = tokens[1];

                            if (model.name.empty())
                            {
                                model.name = model.file;
                            }
                        }
                    }
                    else
                    {
                        ParseTransformField(tokens, model.transform);
                    }
                }
            }

            void ParseLightBlock(std::ifstream &file, SceneLightDesc &light)
            {
                ConsumeOpeningBraceIfNeeded(file);

                std::string line;

                while (std::getline(file, line))
                {
                    line = StripInlineComment(Trim(line));

                    if (IsCommentOrEmpty(line))
                    {
                        continue;
                    }

                    if (IsBlockEnd(line))
                    {
                        break;
                    }

                    std::vector<std::string> tokens = Tokenize(line);

                    if (tokens.empty())
                    {
                        continue;
                    }

                    const std::string key = ToLower(tokens[0]);

                    if (key == "type")
                    {
                        if (tokens.size() >= 2)
                        {
                            light.type = ParseLightType(tokens[1]);
                        }
                    }
                    else if (key == "position")
                    {
                        ReadVec3(tokens, light.position);
                    }
                    else if (key == "direction")
                    {
                        ReadVec3(tokens, light.direction);
                    }
                    else if (key == "emission" || key == "color")
                    {
                        ReadVec3(tokens, light.emission);
                    }
                    else if (key == "intensity")
                    {
                        ReadFloat(tokens, light.intensity);
                    }
                    else if (key == "radius")
                    {
                        ReadFloat(tokens, light.radius);
                    }
                    else if (key == "u")
                    {
                        ReadVec3(tokens, light.u);
                    }
                    else if (key == "v")
                    {
                        ReadVec3(tokens, light.v);
                    }
                }
            }

        private:
            std::filesystem::path m_sceneFile;
        };
    } // namespace

    bool SceneFileLoader::Load(const std::filesystem::path &sceneFile, SceneLoadDesc &outDesc)
    {
        SceneParser parser(sceneFile);
        return parser.Parse(outDesc);
    }
} // namespace ElecNeko
