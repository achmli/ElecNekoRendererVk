#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outMaterial;

layout(binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
    mat4 viewNoTrans;
    mat4 invView;
    mat4 invProj;
} ubo;

layout(binding = 1) uniform LightParms {
    vec2 viewport;
    vec2 _pad0;
    vec3 sunDir;
    float sunIntensity;
    vec3 sunColor;
    float sunAngularRadius;
    float sunGlowSpread;
    float explosure;
} lightParms;

layout(binding = 2) uniform SkyParms {
    vec3 skyColor;
    float zenithBrightenA;
    float horizonBrightenB;
    float baseSkyBrightnessC;
    float exponentialScatteringD;
    float circumsolarGlowE;
    float angularScatteringF;
    float HG;
    float HGParmH;
    float horizonFalloffI;
    float lm;
} skyParms;

const float PI = 3.1415926535898;

float HenyeyGreenstein(float g, float cosGamma) {
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosGamma;
    denom=max(denom, 1e-3);
    return (1.0 - g2) / (4.0 * 3.14159265 * pow(denom, 1.5));
}

float chi(float g, float cosGamma) {
    float denom = 1.0 + g * g - 2.0 * g * cosGamma;

    denom = max(denom, 1e-5);

    return (1.0 - g * g) / (denom * sqrt(denom));
}

void main(){
    vec2 ndc = (gl_FragCoord.xy / lightParms.viewport) * 2.0 - 1.0;
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
    float cosGamma = dot(worldDir, normalize(lightParms.sunDir));
    // float cosGamma = dot(worldDir, ubo.sunDir);
    cosGamma=clamp(cosGamma, -1.0, 1.0);
    float gamma = acos(cosGamma);

    // attennuation simplified F(theta, gamma) (stable)
    float FHorizon = 1.0 + skyParms.zenithBrightenA * exp(skyParms.horizonBrightenB / (cosTheta+0.01));

    // scattering
    float scattering = skyParms.baseSkyBrightnessC + skyParms.exponentialScatteringD * exp(skyParms.circumsolarGlowE * gamma);

    scattering += skyParms.angularScatteringF * (cosGamma * cosGamma);

    scattering += skyParms.HG * HenyeyGreenstein(skyParms.HGParmH, cosGamma);

    scattering += skyParms.horizonFalloffI * sqrt(cosTheta);

    float Fval = FHorizon * scattering;
    Fval = clamp(Fval, 0.0, 6.0);

    // base sky radiance (linear)
    vec3 skyRadiance = skyParms.skyColor * (Fval * skyParms.lm);

    // sun core and glow
    float r = max(lightParms.sunAngularRadius, 1e-5);
    float glow = max(lightParms.sunGlowSpread, r * 2.0);

    // gaussian disk (sharp core)
    float coreSigma = max(r * 0.6, 1e-6);// smaller sigma than radius for crisp edge
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
    vec3 sunRadiance = lightParms.sunColor * (lightParms.sunIntensity * sunShape);

    // sunRadiance += ubo.sunColor * (sunIntensityScaled * 0.25 * core);

    // add sun radiance to sky radiance
    vec3 hdrColor = skyRadiance + sunRadiance;

    // hdrColor = hdrColor / (hdrColor + vec3(1.0));
    //skyRadiance = pow(skyRadiance, vec3(1.0/2.2));
    outColor = vec4(hdrColor, 1.0);
    outNormal = vec4(0.0, 0.0, 1.0, 0.0);
    outMaterial = vec4(0.0);
}