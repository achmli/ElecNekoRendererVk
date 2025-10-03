#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outMaterial;
layout(location = 3) out vec4 outLinearDepth;

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

layout (std140, binding = 2) readonly buffer Materials {
    Material materials[];
};

layout(binding = 3) uniform sampler2DArray textureArray;

layout(location = 0) in vec3 worldNormal;
layout(location = 1) in vec2 uv;
layout(location = 2) in flat int materialId;
layout(location = 3) in vec3 inViewPos;

vec3 GetNormalFromMap(vec3 n, vec3 nm) {
    nm = nm * 2.0 - 1.0;
    // build TBN from normal only approximation
    vec3 N = normalize(n);
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * nm);
}

void main() {
    Material mat = materials[materialId];

    vec3 N = normalize(worldNormal);
    if (mat.normalTexId > -1)
    {
        vec4 NM=texture(textureArray, vec3(uv, mat.normalTexId));
        N=GetNormalFromMap(N, NM.xyz);
    }

    vec3 albedo = mat.baseColor;
    if (mat.baseColorTexId > -1)
    {
        vec4 baseColorTex = texture(textureArray, vec3(uv, mat.baseColorTexId));
        albedo = baseColorTex.rgb;
    }

    float metallic = mat.metallic;
    float roughness = mat.roughness;
    if (mat.metalRoughTexId > -1)
    {
        vec4 mr = texture(textureArray, vec3(uv, mat.metalRoughTexId));
        metallic = mix(metallic, mr.r, step(0.001, mr.r));
        roughness = mix(roughness, mr.g, step(0.001, mr.g));
    }
    roughness = clamp(roughness, 0.045, 1.0);
    float specTrans = clamp(mat.specTrans, 0.0, 1.0);
    float anisotropic = clamp(mat.anisotropic, 0.0, 1.0);
    float opacity = clamp(mat.opacity, 0.0, 1.0);

    float linearDepth = -inViewPos.z;

    outColor = vec4(albedo, opacity);
    outNormal = vec4(N, 0.0);
    outMaterial = vec4(metallic, roughness, specTrans, anisotropic);
    outLinearDepth = vec4(linearDepth, 0.0, 0.0, 0.0);
}