// src/Renderer/Scene/RenderSceneBuilder.cpp
#include "Math/Quat.h"

#include "Renderer/Scene/RenderSceneBuilder.h"

#include "Renderer/Assets/AssetManager.h"
#include "Renderer/Assets/MaterialAsset.h"
#include "Renderer/Mesh/StaticMesh.h"
#include "Renderer/Scene/RenderScene.h"

#include "RHI2/RHIDevice.h"
#include "RHI2/RHIUpload.h"

#include "stb_image.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ElecNeko
{
    namespace
    {
        static float Dot4(const float a[4], const float b[4]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]; }

        static SceneAssetMatrix MultiplyMatrix(const SceneAssetMatrix &a, const SceneAssetMatrix &b)
        {
            SceneAssetMatrix out{};

            for (int row = 0; row < 4; ++row)
            {
                for (int col = 0; col < 4; ++col)
                {
                    out.m[row * 4 + col] = a.m[row * 4 + 0] * b.m[0 * 4 + col] + a.m[row * 4 + 1] * b.m[1 * 4 + col] + a.m[row * 4 + 2] * b.m[2 * 4 + col] +
                                           a.m[row * 4 + 3] * b.m[3 * 4 + col];
                }
            }

            return out;
        }

        static SceneAssetMatrix MakeTranslationMatrix(const Vec3 &position)
        {
            SceneAssetMatrix m{};

            m.m[12] = position[0];
            m.m[13] = position[1];
            m.m[14] = position[2];

            return m;
        }

        static SceneAssetMatrix MakeScaleMatrix(const Vec3 &scale)
        {
            SceneAssetMatrix m{};

            m.m[0] = scale[0];
            m.m[5] = scale[1];
            m.m[10] = scale[2];

            return m;
        }

        static SceneAssetMatrix MakeRotationMatrixFromQuaternion(float x, float y, float z, float w)
        {
            SceneAssetMatrix m{};

            const float lenSq = x * x + y * y + z * z + w * w;

            if (lenSq > 0.0f)
            {
                const float invLen = 1.0f / std::sqrt(lenSq);
                x *= invLen;
                y *= invLen;
                z *= invLen;
                w *= invLen;
            }

            const float xx = x * x;
            const float yy = y * y;
            const float zz = z * z;
            const float xy = x * y;
            const float xz = x * z;
            const float yz = y * z;
            const float wx = w * x;
            const float wy = w * y;
            const float wz = w * z;

            m.m[0] = 1.0f - 2.0f * (yy + zz);
            m.m[1] = 2.0f * (xy - wz);
            m.m[2] = 2.0f * (xz + wy);
            m.m[3] = 0.0f;

            m.m[4] = 2.0f * (xy + wz);
            m.m[5] = 1.0f - 2.0f * (xx + zz);
            m.m[6] = 2.0f * (yz - wx);
            m.m[7] = 0.0f;

            m.m[8] = 2.0f * (xz - wy);
            m.m[9] = 2.0f * (yz + wx);
            m.m[10] = 1.0f - 2.0f * (xx + yy);
            m.m[11] = 0.0f;

            m.m[12] = 0.0f;
            m.m[13] = 0.0f;
            m.m[14] = 0.0f;
            m.m[15] = 1.0f;

            return m;
        }

        static SceneAssetMatrix MakeMatrixFromSceneTransform(const SceneTransformDesc &transform)
        {
            const SceneAssetMatrix t = MakeTranslationMatrix(transform.position);
            const SceneAssetMatrix r = MakeRotationMatrixFromQuaternion(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
            const SceneAssetMatrix s = MakeScaleMatrix(transform.scale);

            // Legacy scene transform convention is assumed to be T * R * S.
            return MultiplyMatrix(MultiplyMatrix(t, r), s);
        }

        static Mat4 ConvertSceneAssetMatrixToMat4(const SceneAssetMatrix &src)
        {
            Mat4 dst{};

            static_assert(sizeof(Mat4) >= sizeof(float) * 16, "Mat4 must contain at least 16 floats.");

            std::memcpy(&dst, src.m, sizeof(float) * 16);

            return dst;
        }

        static MaterialAlphaMode GetMaterialAlphaMode(const MaterialAssetDesc &desc) { return desc.alphaMode; }

        static AlphaMode ConvertToLegacyAlphaMode(MaterialAlphaMode mode)
        {
            switch (mode)
            {
                case MaterialAlphaMode::Mask:
                    return AlphaMode::Mask;
                case MaterialAlphaMode::Blend:
                    return AlphaMode::Blend;
                case MaterialAlphaMode::Opaque:
                default:
                    return AlphaMode::Opaque;
            }
        }

        struct TextureArrayBuildResult
        {
            uint32_t width = 1;
            uint32_t height = 1;
            uint32_t layers = 1;

            std::vector<uint8_t> rgba8Pixels;

            std::unordered_map<uint32_t, uint32_t> textureToLayer;

            std::vector<uint32_t> referencedTextures;
        };

        struct DecodedTextureRGBA8
        {
            bool success = false;

            uint32_t textureIndex = 0;

            int width = 0;
            int height = 0;
            int channels = 4;

            bool srgb = true;

            std::string debugName;
            std::filesystem::path path;

            std::vector<uint8_t> pixels;
        };

        static TextureArrayBuildResult BuildDefaultWhiteTextureArray()
        {
            TextureArrayBuildResult result{};
            result.width = 1;
            result.height = 1;
            result.layers = 1;

            result.rgba8Pixels.resize(4);
            result.rgba8Pixels[0] = 255;
            result.rgba8Pixels[1] = 255;
            result.rgba8Pixels[2] = 255;
            result.rgba8Pixels[3] = 255;

            return result;
        }

        static void AddReferencedTexture(TextureArrayBuildResult &textureArray, TextureHandle texture)
        {
            if (!texture.IsValid())
            {
                return;
            }

            const uint32_t textureIndex = texture.index;

            for (uint32_t existingTextureIndex: textureArray.referencedTextures)
            {
                if (existingTextureIndex == textureIndex)
                {
                    return;
                }
            }

            textureArray.referencedTextures.push_back(textureIndex);
        }

        static void CollectMaterialReferencedTextures(TextureArrayBuildResult &textureArray, const MaterialAsset &material)
        {
            AddReferencedTexture(textureArray, material.desc.baseColorTexture);
            AddReferencedTexture(textureArray, material.desc.normalTexture);
            AddReferencedTexture(textureArray, material.desc.metalRoughTexture);
            AddReferencedTexture(textureArray, material.desc.emissionTexture);
        }

        static DecodedTextureRGBA8 DecodeTextureAssetToRGBA8(uint32_t textureIndex, const TextureAsset &textureAsset)
        {
            DecodedTextureRGBA8 result{};
            result.textureIndex = textureIndex;
            result.srgb = textureAsset.desc.srgb;
            result.debugName = textureAsset.desc.debugName;
            result.path = textureAsset.desc.path;

            if (textureAsset.desc.sourceType != TextureSourceType::File)
            {
                return result;
            }

            const std::string filename = textureAsset.desc.path.string();

            int width = 0;
            int height = 0;
            int originalChannels = 0;

            constexpr int requestedChannels = 4;

            uint8_t *pixels = stbi_load(filename.c_str(), &width, &height, &originalChannels, requestedChannels);

            if (pixels == nullptr || width <= 0 || height <= 0)
            {
                if (pixels != nullptr)
                {
                    stbi_image_free(pixels);
                }

                printf("[RenderSceneBuilder][TextureDecode] failed index=%u path=%s\n", textureIndex, filename.c_str());

                return result;
            }

            const size_t byteSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(requestedChannels);

            result.width = width;
            result.height = height;
            result.channels = requestedChannels;
            result.pixels.assign(pixels, pixels + byteSize);
            result.success = true;

            stbi_image_free(pixels);

            return result;
        }

        static std::vector<uint8_t> ResizeRGBA8Nearest(const uint8_t *srcPixels, int srcWidth, int srcHeight, int dstWidth, int dstHeight)
        {
            std::vector<uint8_t> dstPixels(static_cast<size_t>(dstWidth) * static_cast<size_t>(dstHeight) * 4);

            if (srcPixels == nullptr || srcWidth <= 0 || srcHeight <= 0 || dstWidth <= 0 || dstHeight <= 0)
            {
                return dstPixels;
            }

            for (int y = 0; y < dstHeight; ++y)
            {
                const int srcY = y * srcHeight / dstHeight;

                for (int x = 0; x < dstWidth; ++x)
                {
                    const int srcX = x * srcWidth / dstWidth;

                    const size_t srcOffset = (static_cast<size_t>(srcY) * static_cast<size_t>(srcWidth) + static_cast<size_t>(srcX)) * 4;

                    const size_t dstOffset = (static_cast<size_t>(y) * static_cast<size_t>(dstWidth) + static_cast<size_t>(x)) * 4;

                    dstPixels[dstOffset + 0] = srcPixels[srcOffset + 0];
                    dstPixels[dstOffset + 1] = srcPixels[srcOffset + 1];
                    dstPixels[dstOffset + 2] = srcPixels[srcOffset + 2];
                    dstPixels[dstOffset + 3] = srcPixels[srcOffset + 3];
                }
            }

            return dstPixels;
        }

        static void BuildTextureArrayFromReferencedTextures(TextureArrayBuildResult &textureArray, const AssetManager &assetManager)
        {
            std::vector<DecodedTextureRGBA8> decodedTextures;

            decodedTextures.reserve(textureArray.referencedTextures.size());

            for (uint32_t textureIndex: textureArray.referencedTextures)
            {
                TextureHandle textureHandle{};
                textureHandle.index = textureIndex;

                const TextureAsset &textureAsset = assetManager.GetTexture(textureHandle);

                DecodedTextureRGBA8 decoded = DecodeTextureAssetToRGBA8(textureIndex, textureAsset);

                if (!decoded.success)
                {
                    printf("[RenderSceneBuilder][TextureArray] skip failed texture index=%u path=%s\n", textureIndex,
                           textureAsset.desc.path.generic_string().c_str());

                    continue;
                }

                decodedTextures.push_back(std::move(decoded));
            }

            if (decodedTextures.empty())
            {
                // Keep default 1x1 white array.
                return;
            }

            const uint32_t targetWidth = static_cast<uint32_t>(decodedTextures[0].width);

            const uint32_t targetHeight = static_cast<uint32_t>(decodedTextures[0].height);

            const uint32_t layerCount = 1u + static_cast<uint32_t>(decodedTextures.size());

            textureArray.width = targetWidth;
            textureArray.height = targetHeight;
            textureArray.layers = layerCount;

            const size_t layerByteSize = static_cast<size_t>(targetWidth) * static_cast<size_t>(targetHeight) * 4;

            textureArray.rgba8Pixels.clear();
            textureArray.rgba8Pixels.resize(layerByteSize * static_cast<size_t>(layerCount));

            // Layer 0: default white fallback.
            for (size_t i = 0; i < layerByteSize; i += 4)
            {
                textureArray.rgba8Pixels[i + 0] = 255;
                textureArray.rgba8Pixels[i + 1] = 255;
                textureArray.rgba8Pixels[i + 2] = 255;
                textureArray.rgba8Pixels[i + 3] = 255;
            }

            textureArray.textureToLayer.clear();

            uint32_t nextLayer = 1;

            for (const DecodedTextureRGBA8 &decoded: decodedTextures)
            {
                std::vector<uint8_t> resizedPixels;

                const bool sameSize = decoded.width == static_cast<int>(targetWidth) && decoded.height == static_cast<int>(targetHeight);

                if (sameSize)
                {
                    resizedPixels = decoded.pixels;
                }
                else
                {
                    resizedPixels = ResizeRGBA8Nearest(decoded.pixels.data(), decoded.width, decoded.height, static_cast<int>(targetWidth),
                                                       static_cast<int>(targetHeight));
                }

                const size_t dstOffset = layerByteSize * static_cast<size_t>(nextLayer);

                std::memcpy(textureArray.rgba8Pixels.data() + dstOffset, resizedPixels.data(), layerByteSize);

                textureArray.textureToLayer[decoded.textureIndex] = nextLayer;

                printf("[RenderSceneBuilder][TextureArray] texture index=%u -> layer=%u size=%ux%u original=%dx%d\n", decoded.textureIndex, nextLayer,
                       targetWidth, targetHeight, decoded.width, decoded.height);

                ++nextLayer;
            }

            printf("[RenderSceneBuilder][TextureArray] built width=%u height=%u layers=%u\n", textureArray.width, textureArray.height, textureArray.layers);
        }

        static void DecodeReferencedTexturesForDebug(const TextureArrayBuildResult &textureArray, const AssetManager &assetManager)
        {
            printf("[RenderSceneBuilder] referencedTextures=%zu\n", textureArray.referencedTextures.size());

            for (uint32_t textureIndex: textureArray.referencedTextures)
            {
                TextureHandle textureHandle{};
                textureHandle.index = textureIndex;

                const TextureAsset &textureAsset = assetManager.GetTexture(textureHandle);

                DecodedTextureRGBA8 decoded = DecodeTextureAssetToRGBA8(textureIndex, textureAsset);

                printf("  [TextureDecode] index=%u ok=%d size=%dx%d srgb=%d path=%s debugName=%s\n", textureIndex, decoded.success ? 1 : 0, decoded.width,
                       decoded.height, decoded.srgb ? 1 : 0, textureAsset.desc.path.generic_string().c_str(), textureAsset.desc.debugName.c_str());
            }
        }

        static const char *TextureSourceTypeName(TextureSourceType type)
        {
            switch (type)
            {
                case TextureSourceType::File:
                    return "File";
                case TextureSourceType::Memory:
                    return "Memory";
                case TextureSourceType::DefaultWhite:
                    return "DefaultWhite";
                case TextureSourceType::DefaultNormal:
                    return "DefaultNormal";
                case TextureSourceType::DefaultMetalRough:
                    return "DefaultMetalRough";
                case TextureSourceType::DefaultBlack:
                    return "DefaultBlack";
                default:
                    return "Unknown";
            }
        }

        static void PrintReferencedTextures(const TextureArrayBuildResult &textureArray, const AssetManager &assetManager)
        {
            printf("[RenderSceneBuilder] referencedTextures=%zu\n", textureArray.referencedTextures.size());

            for (uint32_t textureIndex: textureArray.referencedTextures)
            {
                TextureHandle textureHandle{};
                textureHandle.index = textureIndex;

                const TextureAsset &textureAsset = assetManager.GetTexture(textureHandle);

                const std::string path = textureAsset.desc.path.generic_string();

                printf("  [Texture] index=%u source=%s path=%s debugName=%s size=%dx%d channels=%d srgb=%d\n", textureIndex,
                       TextureSourceTypeName(textureAsset.desc.sourceType), path.c_str(), textureAsset.desc.debugName.c_str(), textureAsset.width,
                       textureAsset.height, textureAsset.channels, textureAsset.desc.srgb ? 1 : 0);
            }
        }

        static int32_t FindTextureLayerOrInvalid(const TextureArrayBuildResult &textureArray, TextureHandle texture)
        {
            if (!texture.IsValid())
            {
                return -1;
            }

            const auto it = textureArray.textureToLayer.find(texture.index);

            if (it == textureArray.textureToLayer.end())
            {
                return -1;
            }

            return static_cast<int32_t>(it->second);
        }

        static Material ConvertMaterialAssetToLegacyMaterial(const MaterialAsset &asset, const TextureArrayBuildResult &textureArray)
        {
            const MaterialAssetDesc &src = asset.desc;

            Material dst{};
            dst.name = src.name;

            dst.baseColor = src.baseColor;
            dst.emission = src.emission;

            dst.metallic = src.metallic;
            dst.roughness = src.roughness;
            dst.specularTint = src.specularTint;
            dst.specTrans = src.specTrans;
            dst.anisotropic = src.anisotropic;
            dst.subsurface = src.subsurface;

            dst.sheen = src.sheen;
            dst.sheenTint = src.sheenTint;

            dst.clearcoat = src.clearcoat;
            dst.clearcoatGloss = src.clearcoatGloss;

            dst.ior = src.ior;

            dst.opacity = src.opacity;
            dst.alphaMode = ConvertToLegacyAlphaMode(src.alphaMode);
            dst.alphaCutoff = src.alphaCutoff;

            dst.baseColorTexId = FindTextureLayerOrInvalid(textureArray, src.baseColorTexture);

            dst.metallicRoughtnessTexId = FindTextureLayerOrInvalid(textureArray, src.metalRoughTexture);

            dst.normalMapTexId = FindTextureLayerOrInvalid(textureArray, src.normalTexture);

            dst.emissionmapTexId = FindTextureLayerOrInvalid(textureArray, src.emissionTexture);

            return dst;
        }

        static uint32_t ResolveSectionMaterialIndex(const StaticMeshSection &section, const MaterialSet &materialSet)
        {
            const uint32_t materialSlot = section.materialIndex;

            if (materialSlot < materialSet.slots.size())
            {
                const MaterialHandle materialHandle = materialSet.slots[materialSlot];

                if (materialHandle.IsValid())
                {
                    return materialHandle.index;
                }
            }

            if (!materialSet.slots.empty() && materialSet.slots[0].IsValid())
            {
                return materialSet.slots[0].index;
            }

            return 0;
        }

        static bool UploadStaticMesh(RHI::Device *rhiDevice, RHI::UploadBatch *uploadBatch, const StaticMeshAsset &asset,
                                     std::unique_ptr<StaticMeshGPU> &outMesh)
        {
            outMesh = std::make_unique<StaticMeshGPU>();

            const bool ok = outMesh->Upload(rhiDevice, uploadBatch, asset);
            if (!ok)
            {
                outMesh.reset();
                return false;
            }

            return true;
        }

        static Mat4 MakeLegacyMat4FromSceneTransform(const SceneTransformDesc &transform)
        {
            Mat4 translate;
            Mat4 scale;
            Mat4 rotation;

            translate.Identity();
            scale.Identity();
            rotation.Identity();

            translate.rows[3] = Vec4(transform.position.x, transform.position.y, transform.position.z, 1.0f);

            scale.rows[0].x = transform.scale.x;
            scale.rows[1].y = transform.scale.y;
            scale.rows[2].z = transform.scale.z;

            Quat q;
            q.x = transform.rotation.x;
            q.y = transform.rotation.y;
            q.z = transform.rotation.z;
            q.w = transform.rotation.w;
            rotation = q.ToMat4();

            // Match the old World.cpp scene transform convention.
            return scale * rotation * translate;
        }

        static Mat4 ConvertSceneAssetMatrixToLegacyMat4(const SceneAssetMatrix &src)
        {
            Mat4 out;
            out.rows[0] = Vec4(src.m[0], src.m[1], src.m[2], src.m[3]);
            out.rows[1] = Vec4(src.m[4], src.m[5], src.m[6], src.m[7]);
            out.rows[2] = Vec4(src.m[8], src.m[9], src.m[10], src.m[11]);
            out.rows[3] = Vec4(src.m[12], src.m[13], src.m[14], src.m[15]);
            return out;
        }
    } // namespace

    RenderSceneBuilderResult RenderSceneBuilder::BuildRenderScene(RHI::Device *rhiDevice, const SceneAssetBuildResult &buildData, AssetManager &assetManager,
                                                                  const RenderSceneBuilderOptions &options)
    {
        RenderSceneBuilderResult result{};

        if (rhiDevice == nullptr)
        {
            result.success = false;
            result.errorMessage = "RHI device is null.";
            return result;
        }

        std::unique_ptr<RenderScene> renderScene = std::make_unique<RenderScene>();

        TextureArrayBuildResult textureArray = BuildDefaultWhiteTextureArray();

        for (uint32_t materialIndex = 0; materialIndex < static_cast<uint32_t>(assetManager.GetMaterialCount()); ++materialIndex)
        {
            MaterialHandle materialHandle{};
            materialHandle.index = materialIndex;

            const MaterialAsset &materialAsset = assetManager.GetMaterial(materialHandle);

            CollectMaterialReferencedTextures(textureArray, materialAsset);
        }

        DecodeReferencedTexturesForDebug(textureArray, assetManager);

        if (options.enableTextures)
        {
            BuildTextureArrayFromReferencedTextures(textureArray, assetManager);
        }

        renderScene->materials.reserve(assetManager.GetMaterialCount());

        for (uint32_t materialIndex = 0; materialIndex < static_cast<uint32_t>(assetManager.GetMaterialCount()); ++materialIndex)
        {
            MaterialHandle materialHandle{};
            materialHandle.index = materialIndex;

            const MaterialAsset &materialAsset = assetManager.GetMaterial(materialHandle);

            Material material = ConvertMaterialAssetToLegacyMaterial(materialAsset, textureArray);

            renderScene->materials.push_back(material);
        }

        std::unordered_map<uint32_t, uint32_t> meshHandleToRenderMeshIndex;
        meshHandleToRenderMeshIndex.reserve(assetManager.GetStaticMeshCount());

        std::unique_ptr<RHI::UploadBatch> uploadBatch = rhiDevice->CreateUploadBatch();

        if (!uploadBatch)
        {
            result.success = false;
            result.errorMessage = "Failed to create RHI upload batch.";
            return result;
        }

        const bool uploadBatchStarted = uploadBatch->Begin();

        if (!uploadBatchStarted)
        {
            result.success = false;
            result.errorMessage = "Failed to begin Vulkan upload batch.";
            return result;
        }

        for (const SceneAssetMeshInstance &assetInstance: buildData.meshInstances)
        {
            if (!assetInstance.mesh.IsValid())
            {
                continue;
            }

            if (meshHandleToRenderMeshIndex.find(assetInstance.mesh.index) != meshHandleToRenderMeshIndex.end())
            {
                continue;
            }

            const StaticMeshAsset &meshAsset = assetManager.GetStaticMesh(assetInstance.mesh);

            std::unique_ptr<StaticMeshGPU> meshGPU;

            if (!UploadStaticMesh(rhiDevice, uploadBatch.get(), meshAsset, meshGPU))
            {
                result.success = false;
                result.errorMessage = "Failed to upload static mesh: " + meshAsset.name;
                return result;
            }

            const uint32_t renderMeshIndex = renderScene->AddMesh(std::move(meshGPU));

            meshHandleToRenderMeshIndex.emplace(assetInstance.mesh.index, renderMeshIndex);
        }

        renderScene->meshInstances.reserve(buildData.meshInstances.size());

        for (const SceneAssetMeshInstance &assetInstance: buildData.meshInstances)
        {
            if (!assetInstance.mesh.IsValid() || !assetInstance.materialSet.IsValid())
            {
                continue;
            }

            auto meshIt = meshHandleToRenderMeshIndex.find(assetInstance.mesh.index);

            if (meshIt == meshHandleToRenderMeshIndex.end())
            {
                continue;
            }

            const StaticMeshAsset &meshAsset = assetManager.GetStaticMesh(assetInstance.mesh);

            const MaterialSet &materialSet = assetManager.GetMaterialSet(assetInstance.materialSet);

            Mat4 rootMatrix = MakeLegacyMat4FromSceneTransform(assetInstance.transform);

            Mat4 finalMatrix = rootMatrix;

            if (assetInstance.hasLocalToModel)
            {
                Mat4 localToModel = ConvertSceneAssetMatrixToLegacyMat4(assetInstance.localToModel);

                // Match old World path convention as closely as possible:
                // scene transform is the root transform, imported node transform is local.
                finalMatrix = localToModel * rootMatrix;
            }

            MeshInstance instance{};
            instance.meshIndex = meshIt->second;
            instance.localToWorld = finalMatrix;
            instance.worldToLocal = instance.localToWorld.Inverse();
            instance.visible = assetInstance.visible;

            const StaticMeshGPU *meshGPU = renderScene->meshes[instance.meshIndex].get();

            if (meshGPU != nullptr)
            {
                instance.materialOverrides.reserve(meshGPU->sections.size());

                for (const StaticMeshSection &section: meshGPU->sections)
                {
                    const uint32_t materialIndex = ResolveSectionMaterialIndex(section, materialSet);

                    instance.materialOverrides.push_back(materialIndex);
                }
            }
            else if (!materialSet.slots.empty() && materialSet.slots[0].IsValid())
            {
                instance.materialOverrides.push_back(materialSet.slots[0].index);
            }

            renderScene->AddMeshInstance(instance);
        }

        renderScene->BuildDrawLists();

        const bool textureArrayOk = renderScene->CreateTextureArrayFromRGBA8Pixels(
                rhiDevice, uploadBatch.get(), textureArray.width, textureArray.height, textureArray.layers, textureArray.rgba8Pixels.data(),
                static_cast<uint64_t>(textureArray.rgba8Pixels.size()), "RenderScene.TextureArray");

        if (!textureArrayOk)
        {
            result.success = false;
            result.errorMessage = "Failed to create default texture array.";
            return result;
        }

        const bool uploadBatchSubmitted = uploadBatch->SubmitAndWait();

        if (!uploadBatchSubmitted)
        {
            result.success = false;
            result.errorMessage = "Failed to submit Vulkan upload batch.";
            return result;
        }

        if (options.uploadGpuSceneBuffers)
        {
            const bool uploadOk = renderScene->UploadGpuSceneBuffers(rhiDevice);

            if (!uploadOk)
            {
                result.success = false;
                result.errorMessage = "Failed to upload RenderScene GPU scene buffers.";
                return result;
            }
        }

        result.success = true;
        result.renderScene = std::move(renderScene);

        printf("[RenderSceneBuilder] success=1 meshes=%zu instances=%zu materials=%zu opaque=%zu masked=%zu transparent=%zu shadow=%zu\n",
               result.renderScene->meshes.size(), result.renderScene->meshInstances.size(), result.renderScene->materials.size(),
               result.renderScene->drawList.opaque.size(), result.renderScene->drawList.masked.size(), result.renderScene->drawList.transparent.size(),
               result.renderScene->drawList.shadow.size());

        return result;
    }
} // namespace ElecNeko
