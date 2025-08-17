#version 450

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
    mat4 viewNoTrans;
    mat4 invView;
    mat4 invProj;
    vec2 viewport;
    vec2 _pad0;
    vec3 sunDir;
    float sunIntensity;
    vec3 sunColor;
    float sunAngularRadius;
    float sunGlowSpread;
    float explosure;
} ubo;

const vec3 BASE_SKY_TINT = vec3(0.55, 0.7, 1.0);
const float A = 0.08;     
const float B = -0.35;    
const float C = 0.22;     
const float D = 0.1;     
const float E = -2.2;     
const float F = 0.18;      
const float G = 0.02;     
const float H = 0.75;    
const float I = 0.1;
const float Lm = 1.5;

const float PI = 3.1415926535898;

// const vec3 RAYLEIGH_BETA = vec3(5.8e-6, 13.5e-6, 33.1e-6);

// true Henyey-Greenstein phase function
float HenyeyGreenstein(float g, float cosGamma) {
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosGamma;
    denom=max(denom, 1e-3);
    return (1.0 - g2) / (4.0 * 3.14159265 * pow(denom, 1.5));
}

float chi(float g, float cosGamma) {
    // float g2 = g * g;
    // float cosGamma = cos(gamma);
    // return (1.0 - g2) / pow(1.0 + g2 - 2.0 * g * cosGamma, 1.5);
    float denom = 1.0 + g * g - 2.0 * g * cosGamma;

    denom = max(denom, 1e-5);

    return (1.0 - g * g) / (denom * sqrt(denom));
}

void main() {
    //vec3 view = normalize(viewDir);

    vec2 ndc = (gl_FragCoord.xy / ubo.viewport) * 2.0 - 1.0;
    float clipZ = 1.0;
    vec4 clipPos = vec4(ndc, clipZ, 1.0);
    vec4 viewPos = ubo.invProj * clipPos;
    viewPos /= viewPos.w;

    vec3 viewDir = normalize(viewPos.xyz);
    vec3 worldDir = normalize((ubo.invView * vec4(viewDir, 0.0)).xyz);

    // calculate theta
    float cosTheta=clamp(worldDir.y, 0.0, 1.0);
    float theta = acos(cosTheta);

    // calculate gamma
    float cosGamma = dot(worldDir, normalize(ubo.sunDir));
    // float cosGamma = dot(worldDir, ubo.sunDir);
    cosGamma=clamp(cosGamma, -1.0, 1.0);
    float gamma = acos(cosGamma);

    // attennuation simplified F(theta, gamma) (stable)
    float FHorizon = 1.0 + A * exp(B / (cosTheta+0.01));

    // scattering
    float scattering = C + D * exp(E * gamma);

    scattering += F * (cosGamma * cosGamma);

    scattering += G * HenyeyGreenstein(H, cosGamma);

    scattering += I * sqrt(cosTheta);

    float Fval = FHorizon * scattering;
    Fval = clamp(Fval, 0.0, 6.0);

    // base sky radiance (linear)
    vec3 skyRadiance = BASE_SKY_TINT * (Fval * Lm);

    // sun core and glow
    float r = max(ubo.sunAngularRadius, 1e-5);
    float glow = max(ubo.sunGlowSpread, r * 2.0);

    // gaussian disk (sharp core)
    float coreSigma = max(r * 0.6, 1e-6); // smaller sigma than radius for crisp edge
    float core = exp(- (gamma * gamma) / (2.0 * coreSigma * coreSigma));

    // glow (longer tail)
    // float glowSigma = max(r * 6.0, coreSigma * 2.0);
    // float glowFactor = exp(- (gamma * gamma) / (2.0 * glowSigma * glowSigma));
    float glowFactor = exp(- (gamma) / (glow));

    // combine and clamp
    // float coreWeight = 1.0;
    // float glowWeight = 0.5; // glow weaker than core
    // float sunShape = coreWeight * core + glowWeight * glowFactor;
    float sunShape = clamp(core + 0.12 * glowFactor, 0.0, 20.0);

    // compute sun radiance
    // float sunIntensityScaled = clamp(ubo.sunIntensity, 0.0, 50.0);
    vec3 sunRadiance = ubo.sunColor * (ubo.sunIntensity * sunShape);

    // sunRadiance += ubo.sunColor * (sunIntensityScaled * 0.25 * core);

    // add sun radiance to sky radiance
    vec3 hdrColor = skyRadiance + sunRadiance;

    // hdrColor = hdrColor / (hdrColor + vec3(1.0));
    //skyRadiance = pow(skyRadiance, vec3(1.0/2.2));
    outColor = vec4(hdrColor, 1.0);
}