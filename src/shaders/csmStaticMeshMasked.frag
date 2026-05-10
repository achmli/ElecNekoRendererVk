#version 450

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
    int mediumType;
    float mediumDensity;

    vec3 mediumColor;
    float mediumAnisotropy;

    int baseColorTexId;
    int metalRoughTexId;
    int normalTexId;
    int emissionTexId;

    float opacity;
    int alphaMode;
    float alphaCutoff;
    float padding1;
};

layout(std140, binding = 3) readonly buffer Materials {
    Material materials[];
};

layout(binding = 4) uniform sampler2DArray textureArray;

layout(location = 0) in vec2 uv;
layout(location = 1) flat in uint materialId;

void main() {
    Material material = materials[materialId];

    float alpha = material.opacity;

    if (material.baseColorTexId > -1) {
        alpha *= texture(textureArray, vec3(uv, material.baseColorTexId)).a;
    }

    float cutoff = material.alphaCutoff;

    if (cutoff <= 0.0) {
        cutoff = 0.5;
    }

    if (alpha < cutoff) {
        discard;
    }
}
