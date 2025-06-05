#version 450
/*
    ==========================================
uniforms
    ==========================================
*/

layout(binding = 3) uniform sampler2D texShadow;
layout(binding = 4) uniform sampler2D texAlbedo;

/*
    ==========================================
input
    ==========================================
*/

layout(location = 0) in vec4 worldNormal;
layout(location = 1) in vec4 modelPos;
layout(location = 2) in vec3 modelNormal;
layout(location = 3) in vec4 shadowPos;
layout(location = 4) in vec2 texCoord;

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

/*
    ==========================================
main
    ==========================================
*/
void main() {
    vec3 dirToLight = normalize(vec3(1, 1, 1));

    vec4 finalColor=texture(texAlbedo, texCoord);

    if(finalColor.a<0.0001) {
        discard;
    }

    //
    //  Shadow Mapping
    //
    float shadowDepth = shadowPos.z / shadowPos.w;
    vec2 shadowCoord = shadowPos.xy / shadowPos.w;
    shadowCoord *= 0.5;
    shadowCoord += vec2(0.5);
    vec3 shadowSample = texture(texShadow, shadowCoord.xy).rgb;
    float depthDifference = shadowDepth - shadowSample.r;   // positive if in shadow
    depthDifference = max(0.0, depthDifference) * 1000.0;

    float shadowFactor = 0.0;
    float ds = 1.0 / 4096.0;
    float sampleCount = 0.0;

    vec2 sampleArray[9];
    sampleArray[0] = vec2(0, 0);
    sampleArray[1] = vec2(1, 0.1);
    sampleArray[2] = vec2(-0.2, 1);
    sampleArray[3] = vec2(-1, 0.2);
    sampleArray[4] = vec2(0.3, -1);

    sampleArray[5] = vec2(1, 1);
    sampleArray[6] = vec2(-1, 1);
    sampleArray[7] = vec2(-1, -1);
    sampleArray[8] = vec2(1, -1);

    float angle = randAngle();
    mat2 rotator;
    rotator[0][0] = cos(angle);
    rotator[0][1] = sin(angle);
    rotator[1][0] = -sin(angle);
    rotator[1][1] = cos(angle);
    for (int i = 0; i < 9; i++) {
        vec2 uv = shadowCoord.xy + rotator * sampleArray[i] * ds;
        vec3 shadowSample = texture(texShadow, uv).rgb;
        if (shadowDepth > shadowSample.r) {
            shadowFactor += 1.0;    // in shadow
        }
        sampleCount += 1.0;
    }

    shadowFactor /= sampleCount;
    shadowFactor = 1.0 - shadowFactor;

    float ambient = 0.5;
    float flux = clamp(dot(worldNormal.xyz, dirToLight.xyz), 0.0, 1.0 - ambient) * shadowFactor + ambient;
    finalColor.rgb *= flux;

    outColor = finalColor;
}