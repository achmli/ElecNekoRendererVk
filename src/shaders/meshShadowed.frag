#version 450
/*
    ==========================================
uniforms
    ==========================================
*/

layout(binding = 3) uniform UBOMaterial {
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
} uMat;

layout(binding = 4) uniform lightParms {
    vec2 viewport;
    vec2 _pad0;
    vec3 sunDir;
    float sunIntensity;
    vec3 sunColor;
} lightParm;

layout(binding = 5) uniform sampler2D texShadow;

/*
    ==========================================
input
    ==========================================
*/

layout(location = 0) in vec3 worldNormal;
layout(location = 1) in vec3 worldPos;
layout(location = 2) in vec4 shadowPos;
layout(location = 3) in vec3 camPos;

/*
    ==========================================
output
    ==========================================
*/

layout(location = 0) out vec4 outColor;

// Generate a random angle based on the XY XOR hash
float randAngle() {
    uint x = uint(gl_FragCoord.x);
    uint y = uint(gl_FragCoord.y);
    return (30u * x ^ y + 10u * x * y);
}

// helpers for pbr
const float PI = 3.14159265359;

// normal map decode
vec3 getNormalFromMap(vec3 n, vec3 nm) {
    nm = nm * 2.0 - 1.0;
    // build TBN from normal only approximation
    vec3 N = normalize(n);
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * nm);
}

// GGX distribution 
float D_GGX(float NdotH, float rough) {
    float a = rough * rough;
    float a2 = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// Schlick Fresnel
vec3 F_Schlick(vec3 F0, float cosTheta) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Geometry (Smith) with Schlic-GGX
float G_Schlick_GGX(float NdotV, float k) {
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float k) {
    return G_Schlick_GGX(NdotV, k) * G_Schlick_GGX(NdotL, k);
}

float ComputeShadowPCF(vec4 shadowPos, float NdotL) {
    // proj coords
    vec3 proj = shadowPos.xyz / shadowPos.w;
    vec2 uv = proj.xy * 0.5 + vec2(0.5);
    float currentDepth = proj.z;// * 0.5 + 0.5;

    // bounds check;
    if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return 1.0;
    }

    float shadowMapSize = textureSize(texShadow, 0).x;
    // float bias = 2.0 * 1.0 / shadowMapSize;

    float texel = 1.0/max(1.0, shadowMapSize);
    float radius = texel * 1.5;

    float bias = 2.0 * 1.0 / shadowMapSize;
    // float bias = max(0.0005, baseBias * (1.0 - NdotL)* 10.0);

    float sum = 0.0;
    int count = 0;
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            vec2 off = vec2(float(x), float(y)) * radius;
            float sampleDepth = texture(texShadow, uv + off).r;

            if((currentDepth - bias) <= sampleDepth) {
                sum += 1.0;
            }
            count++;
        }
    }

    return sum / float(count);
}

/*
    ==========================================
main
    ==========================================
*/
void main() {
    // base values
    vec3 N = normalize(worldNormal);
    vec3 V = normalize(camPos - worldPos);
    // Ensure sunDir semantic: make L be vector from surface towards light
    // If your sunDir is "direction the sun shines (i.e. towards surface)", then L = normalize(-lightParm.sunDir).
    // If your sunDir is "direction to sun", then L = normalize(lightParm.sunDir).  Adjust accordingly.
    vec3 L = normalize(lightParm.sunDir); // assume sunDir is direction *from* sun to world (common)
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 1e-4);
    float VdotH = max(dot(V, H), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    // material
    vec3 albedo = uMat.baseColor;
    float metallic = clamp(uMat.metallic, 0.0, 1.0);
    float roughness = clamp(uMat.roughness, 0.045, 1.0); // avoid 0

    // Fresnel F0
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Cook-Torrance
    float D = D_GGX(NdotH, roughness);
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G = G_Smith(NdotV, NdotL, k);
    vec3 F = F_Schlick(F0, VdotH);

    vec3 spec = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-6);

    vec3 kd = (1.0 - F) * (1.0 - metallic); // energy conservation
    vec3 diffuse = kd * albedo / PI;

    vec3 radiance = lightParm.sunColor * lightParm.sunIntensity;
    vec3 Lo = (diffuse + spec) * radiance * NdotL;

    vec3 ambient = vec3(0.03) * albedo;
    vec3 emissive = uMat.emission;

    float visibility = 1.0;
    // only compute shadow if NdotL > 0 (otherwise light is behind)
    if (NdotL > 0.0001) {
        visibility = ComputeShadowPCF(shadowPos, NdotL);
    }

    vec3 color = Lo * visibility + ambient + emissive;

    outColor = vec4(color, uMat.opacity);
}