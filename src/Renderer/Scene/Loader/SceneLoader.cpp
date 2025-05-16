//
// Created by ElecNekoSurface on 25-4-5.
//

#include "SceneLoader.h"
#include <filesystem>
#include <fstream>
#include <map>

namespace ElecNeko
{
    bool LoadSceneFromFile(const std::string &filename, std::unique_ptr<Scene> scene, RenderOption &renderOption)
    {
        const std::filesystem::path fileName(filename);

        std::ifstream file(fileName);

        if (!file.is_open())
        {
            printf("Couldn't open %s for reading\n", filename.c_str());
            return false;
        }

        printf("Now Loading...\n");


        std::map<std::string, std::pair<Material, int>> materialMap;
        std::vector<std::string> albedoTex;
        std::vector<std::string> metallicRoughnessTex;
        std::vector<std::string> normalTex;
        std::filesystem::path filePath = fileName.parent_path() / std::filesystem::path();
        std::string path = filePath.generic_string();

        Material defaultMat;
        scene->AddMaterial(defaultMat);

        std::string line;

        while (std::getline(file, line))
        {
            if (!line.empty() && line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::string token;
            iss >> token;

            if (token == "material" || token == "Material")
            {
                std::string name;
                iss >> name;

                Material material;
                std::string albedoTexName = "none";
                std::string metallicRoughnessTexName = "none";
                std::string normalTexName = "none";
                std::string emissionTexName = "none";
                std::string alphaMode = "none";
                std::string mediumType = "none";

                while (std::getline(file, line))
                {
                    if (line.find('}') != std::string::npos)
                        break;

                    std::istringstream innerIss(line);
                    std::string param;
                    innerIss >> param;

                    if (param == "color")
                    {
                        innerIss >> material.baseColor.x >> material.baseColor.y >> material.baseColor.z;
                    }
                    else if (param == "opacity")
                    {
                        innerIss >> material.opacity;
                    }
                    else if (param == "alphamode")
                    {
                        innerIss >> alphaMode;
                    }
                    else if (param == "alphacutoff")
                    {
                        innerIss >> material.alphaCutoff;
                    }
                    else if (param == "emission")
                    {
                        innerIss >> material.emission.x >> material.emission.y >> material.emission.z;
                    }
                    else if (param == "metallic")
                    {
                        innerIss >> material.metallic;
                    }
                    else if (param == "roughness")
                    {
                        innerIss >> material.roughness;
                    }
                    else if (param == "subsurface")
                    {
                        innerIss >> material.subsurface;
                    }
                    else if (param == "speculartint")
                    {
                        innerIss >> material.specularTint;
                    }
                    else if (param == "anisotropic")
                    {
                        innerIss >> material.anisotropic;
                    }
                    else if (param == "sheen")
                    {
                        innerIss >> material.sheen;
                    }
                    else if (param == "sheentint")
                    {
                        innerIss >> material.sheenTint;
                    }
                    else if (param == "clearcoat")
                    {
                        innerIss >> material.clearCoat;
                    }
                    else if (param == "clearcoatgloss")
                    {
                        innerIss >> material.clearCoatGloss;
                    }
                    else if (param == "spectrans")
                    {
                        innerIss >> material.specTrans;
                    }
                    else if (param == "ior")
                    {
                        innerIss >> material.ior;
                    }
                    else if (param == "albedotexture")
                    {
                        innerIss >> albedoTexName;
                    }
                    else if (param == "metallicroughnesstexture")
                    {
                        innerIss >> metallicRoughnessTexName;
                    }
                    else if (param == "normaltexture")
                    {
                        innerIss >> normalTexName;
                    }
                    else if (param == "emissiontexture")
                    {
                        innerIss >> emissionTexName;
                    }
                    else if (param == "mediumtype")
                    {
                        innerIss >> mediumType;
                    }
                    else if (param == "mediumdensity")
                    {
                        innerIss >> material.mediumDensity;
                    }
                    else if (param == "mediumcolor")
                    {
                        innerIss >> material.mediumColor.x >> material.mediumColor.y >> material.mediumColor.z;
                    }
                    else if (param == "mediumanisotropy")
                    {
                        innerIss >> material.mediumAnisotropy;
                    }
                }

                if (albedoTexName != "none")
                {
                    material.baseColorTexId = scene->AddTexture(path + albedoTexName);
                }

                if (metallicRoughnessTexName != "none")
                {
                    material.metallicRoughnessTexId = scene->AddTexture(path + metallicRoughnessTexName);
                }

                if (normalTexName != "none")
                {
                    material.normalMapTexId = scene->AddTexture(path + normalTexName);
                }

                if (emissionTexName != "none")
                {
                    material.emissionMapTexId = scene->AddTexture(path + emissionTexName);
                }

                if (alphaMode == "opaque")
                {
                    material.alphaMode = Opaque;
                }
                else if (alphaMode == "scatter")
                {
                    material.alphaMode = Scatter;
                }
                else if (alphaMode == "emissive")
                {
                    material.alphaMode = Emissive;
                }

                if (!materialMap.contains(name))
                {
                    int id = scene->AddMaterial(material);
                    materialMap[name] = {material, id};
                }
            }

            if (line.find("light") != std::string::npos)
            {
                Light light;
                Vec3 v1, v2;
                std::string lightType = "none";

                while (std::getline(file, line))
                {
                    if (line.find('}') != std::string::npos)
                        break;

                    std::istringstream innerIss(line);
                    std::string param;
                    innerIss >> param;

                    if (param == "position")
                    {
                        innerIss >> light.position.x >> light.position.y >> light.position.z;
                    }
                    else if (param == "emission")
                    {
                        innerIss >> light.emission.x >> light.emission.y >> light.emission.z;
                    }
                    else if (param == "radius")
                    {
                        innerIss >> light.radius;
                    }
                    else if (param == "v1")
                    {
                        innerIss >> v1.x >> v1.y >> v1.z;
                    }
                    else if (param == "v2")
                    {
                        innerIss >> v2.x >> v2.y >> v2.z;
                    }
                    else if (param == "type")
                    {
                        innerIss >> lightType;
                    }
                }

                if (lightType == "quad")
                {
                    light.type = RectLight;
                    light.u = v1 - light.position;
                    light.v = v2 - light.position;
                    light.area = light.u.Cross(light.v).GetMagnitude();
                }
                else if (lightType == "sphere")
                {
                    light.type = SphereLight;
                    light.area = 4.f * PI * light.radius * light.radius;
                }
                else if (lightType == "distant")
                {
                    light.type = DistantLight;
                    light.area = 0.f;
                }

                scene->AddLight(light);
            }

            if (line.find("camera") != std::string::npos)
            {
                Mat4 xform;
                Vec3 position;
                Vec3 lookAt;
                float fov;
                float aperture = 0, focalDist = 1;
                bool matrixProvided = false;

                while (std::getline(file, line))
                {
                    if (line.find('}') != std::string::npos)
                        break;

                    std::istringstream innerIss(line);
                    std::string param;
                    innerIss >> param;

                    if (param == "position")
                    {
                        innerIss >> position.x >> position.y >> position.z;
                    }
                    else if (param == "lookat")
                    {
                        innerIss >> lookAt.x >> lookAt.y >> lookAt.z;
                    }
                    else if (param == "aperture")
                    {
                        innerIss >> aperture;
                    }
                    else if (param == "focaldist")
                    {
                        innerIss >> focalDist;
                    }
                    else if (param == "fov")
                    {
                        innerIss >> fov;
                    }

                    else if (param == "matrix")
                    {
                        innerIss >> xform.rows[0].x >> xform.rows[0].y >> xform.rows[0].z >> xform.rows[0].w >> xform.rows[1].x >> xform.rows[1].y >>
                                xform.rows[1].z >> xform.rows[1].w >> xform.rows[2].x >> xform.rows[2].y >> xform.rows[2].z >> xform.rows[2].w >>
                                xform.rows[3].x >> xform.rows[3].y >> xform.rows[3].z >> xform.rows[3].w;

                        matrixProvided = true;
                    }

                    // delete scene->camera;

                    if (matrixProvided)
                    {
                        Vec3 forward = Vec3(xform.rows[2].x, xform.rows[2].y, xform.rows[2].z);
                        position = Vec3(xform.rows[3].x, xform.rows[3].y, xform.rows[3].z);
                        lookAt = position + forward;
                    }

                    scene->AddCamera(position, lookAt, fov);
                    scene->camera->aperture = aperture;
                    scene->camera->focalDist = focalDist;
                }
            }

            if (line.find("renderer") != std::string::npos)
            {
                std::string envMap = "none";
                std::string enableAces = "none";
                std::string hideEmitters = "none";
                std::string transparentBackground = "none";
                std::string enableBackground = "none";
                std::string independentRenderSize = "none";
                std::string enableTonemap = "none";
                std::string enableRoughnessMollification = "none";
                std::string enableVolumeMIS = "none";
                std::string enableUniformLight = "none";

                while (std::getline(file, line))
                {
                    if (line.find('}') != std::string::npos)
                        break;

                    std::istringstream innerIss(line);
                    std::string param;
                    innerIss >> param;

                    if (param == "envmapfile")
                    {
                        innerIss >> envMap;
                    }
                    else if (param == "resolution")
                    {
                        innerIss >> renderOption.renderResolution.x >> renderOption.renderResolution.y;
                    }
                    else if (param == "windowresolution")
                    {
                        innerIss >> renderOption.windowResolution.x >> renderOption.windowResolution.y;
                    }
                    else if (param == "envmapintensity")
                    {
                        innerIss >> renderOption.envMapIntensity;
                    }
                    else if (param == "enabletonemap")
                    {
                        innerIss >> enableTonemap;
                    }
                    else if (param == "enableaces")
                    {
                        innerIss >> enableAces;
                    }
                    else if (param == "texarraywidth")
                    {
                        innerIss >> renderOption.texArrayWidth;
                    }
                    else if (param == "texarrayheight")
                    {
                        innerIss >> renderOption.texArrayHeight;
                    }
                    else if (param == "hideemitters")
                    {
                        innerIss >> hideEmitters;
                    }
                    else if (param == "enablebackground")
                    {
                        innerIss >> enableBackground;
                    }
                    else if (param == "transparentbackground")
                    {
                        innerIss >> transparentBackground;
                    }
                    else if (param == "backgroundcolor")
                    {
                        innerIss >> renderOption.backgroundCol.x >> renderOption.backgroundCol.y >> renderOption.backgroundCol.z;
                    }
                    else if (param == "independentrendersize")
                    {
                        innerIss >> independentRenderSize;
                    }
                    else if (param == "envmaprotation")
                    {
                        innerIss >> renderOption.envMapRot;
                    }
                    else if (param == "enableroughnessmollification")
                    {
                        innerIss >> enableRoughnessMollification;
                    }
                    else if (param == "roughnessmollificationamt")
                    {
                        innerIss >> renderOption.roughnessMollificationAmt;
                    }
                    else if (param == "enablevolumemis")
                    {
                        innerIss >> enableVolumeMIS;
                    }
                    else if (param == "enableuniformlight")
                    {
                        innerIss >> enableUniformLight;
                    }
                    else if (param == "uniformlightcolor")
                    {
                        innerIss >> renderOption.uniformLightCol.x >> renderOption.uniformLightCol.y >> renderOption.uniformLightCol.z;
                    }
                }

                if (envMap != "none")
                {
                    scene->AddEnvMap(path + envMap);
                    renderOption.enableEnvMap = true;
                }
                else
                {
                    renderOption.enableEnvMap = false;
                }

                if (enableAces == "true")
                {
                    renderOption.enableAces = true;
                }
                else
                {
                    renderOption.enableAces = false;
                }

                if (hideEmitters == "true")
                {
                    renderOption.hideEmitters = true;
                }
                else
                {
                    renderOption.hideEmitters = false;
                }

                if (enableBackground == "true")
                {
                    renderOption.enableBackground = true;
                }
                else
                {
                    renderOption.enableBackground = false;
                }

                if (transparentBackground == "true")
                {
                    renderOption.transparentBackground = true;
                }
                else
                {
                    renderOption.transparentBackground = false;
                }

                if (independentRenderSize == "true")
                {
                    renderOption.independentRenderSize = true;
                }
                else
                {
                    renderOption.independentRenderSize = false;
                }

                if (enableTonemap == "false")
                {
                    renderOption.enableTonemap = false;
                }
                else
                {
                    renderOption.enableTonemap = true;
                }

                if (enableRoughnessMollification == "true")
                {
                    renderOption.enableRoughnessMollification = true;
                }
                else
                {
                    renderOption.enableRoughnessMollification = false;
                }

                if (enableVolumeMIS == "true")
                {
                    renderOption.enableVolumeMIS = true;
                }
                else
                {
                    renderOption.enableVolumeMIS = false;
                }

                if (enableUniformLight == "true")
                {
                    renderOption.enableUniformLight = true;
                }
                else
                {
                    renderOption.enableUniformLight = false;
                }

                if (!renderOption.independentRenderSize)
                {
                    renderOption.windowResolution = renderOption.renderResolution;
                }
            }

            if (line.find("mesh") != std::string::npos)
            {
                std::string fname;
                Quat rotQuat;
                Mat4 xform, translate, rot, scale;
                int material_id = 0;
                std::string meshName = "none";
                bool matrixProvided = false;

                translate.Identity();
                rot.Identity();
                scale.Identity();

                while (std::getline(file, line))
                {
                    if (line.find('}') != std::string::npos)
                        break;

                    std::istringstream innerIss(line);
                    std::string param;
                    innerIss >> param;

                    std::string matName;

                    if (param == "name")
                    {
                        std::string remaining;
                        std::getline(innerIss >> std::ws, remaining);

                        if (size_t endPos = remaining.find_last_of("\t\n"); endPos != std::string::npos)
                        {
                            meshName = remaining.substr(0, endPos);
                        }
                        else
                        {
                            meshName = remaining;
                        }

                        meshName = meshName.substr(meshName.find_first_not_of(' '));
                    }
                    else if (param == "file")
                    {
                        std::string _file;
                        innerIss >> _file;
                        fname = path + _file;
                    }
                    else if (param == "material")
                    {
                        innerIss >> matName;
                        if (materialMap.contains(matName))
                        {
                            material_id = materialMap[matName].second;
                        }
                        else
                        {
                            printf("Could not find material %s", matName.c_str());
                        }
                    }
                    else if (param == "matrix")
                    {
                        innerIss >> xform.rows[0].x >> xform.rows[0].y >> xform.rows[0].z >> xform.rows[0].w >> xform.rows[1].x >> xform.rows[1].y >>
                                xform.rows[1].z >> xform.rows[1].w >> xform.rows[2].x >> xform.rows[2].y >> xform.rows[2].z >> xform.rows[2].w >>
                                xform.rows[3].x >> xform.rows[3].y >> xform.rows[3].z >> xform.rows[3].w;

                        matrixProvided = true;
                    }
                    else if (param == "position")
                    {
                        innerIss >> translate.rows[0].w >> translate.rows[1].w >> translate.rows[2].w;
                    }
                    else if (param == "scale")
                    {
                        innerIss >> scale.rows[0].x >> scale.rows[1].y >> scale.rows[2].z;
                    }
                    else if (param == "rotation")
                    {
                        innerIss >> rotQuat.x >> rotQuat.y >> rotQuat.z >> rotQuat.w;
                        rot = rotQuat.ToMat4();
                    }
                }

                if (!fname.empty())
                {
                    int mesh_id = scene->AddMesh(fname);
                    if (mesh_id != -1)
                    {
                        std::string instanceName;

                        if (meshName != "none")
                        {
                            instanceName = std::string(meshName);
                        }
                        else
                        {
                            std::filesystem::path fiPath(fname);
                            instanceName = fiPath.filename().string();
                        }

                        Mat4 transformMat;
                        if (matrixProvided)
                        {
                            transformMat = xform;
                        }
                        else
                        {
                            transformMat = scale * rot * translate;
                        }

                        MeshInstance instance(instanceName, mesh_id, transformMat, material_id);
                        scene->AddMeshInstance(instance);
                    }
                }
            }
        }
        return true;
    }

} // namespace ElecNeko
