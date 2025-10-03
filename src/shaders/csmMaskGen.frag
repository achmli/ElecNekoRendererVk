#version 450

// layout(binding = 2) uniform sampler2D texAlbedo;
struct Material {
    vec3 baseColor;
    float anisotropic;

    vec3 emission;
    float padding0;

    float metallic;
    float roughness;
    float subsurface;
    float specularTint;

    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatGloss;

    float specTrans;
    float ior;
    uint mediumType;
    float mediumDensity;

    vec3 mediumColor;
    float mediumAnisotropy;

    int baseColorTexId;
    int metallicRoughnessTexId;
    int normalMapTexId;
    int emissionTexId;

    float opacity;
    uint alphaMode;
    float alphaCutoff;
    float padding1;
};

layout(std140, binding = 3) readonly buffer Materials {
    Material materials[];
};

layout(binding = 4) uniform sampler2DArray textureArray;

layout(location = 0) in vec2 uv;
layout(location = 1) flat in uint materialIdx;

void main() {
    Material material = materials[materialIdx];

    float a = 1.0;

    if (material.baseColorTexId > -1)
    a = texture(textureArray, vec3(uv, material.baseColorTexId)).a;

    if (a<0.0001) {
        discard;
    }
}