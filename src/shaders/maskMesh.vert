#version 450
#extension GL_ARB_separate_shader_objects : enable

/*
    ==========================================
uniforms
    ==========================================
*/

layout(binding = 0) uniform uboCamera {
    mat4 view;
    mat4 proj;
    mat4 viewNoTrans;
    mat4 invView;
    mat4 invProj;
} camera;
layout(binding = 1) uniform uboShadow {
    mat4 view;
    mat4 proj;
} shadow;
layout(std140, binding = 2) readonly buffer uboModel {
    mat4 modelMatrix[];
};

/*
    ==========================================
attributes
    ==========================================
*/

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in uint inMaterialIdx;
layout(location = 4) in uint inModelMatrixIdx;

/*
    ==========================================
output
    ==========================================
*/

layout(location = 0) out vec3 worldNormal;
layout(location = 1) out vec3 worldPos;
layout(location = 2) out vec4 shadowPos;
layout(location = 3) out vec3 camPos;
layout(location = 4) out vec2 texCoords;
layout(location = 5) flat out uint materialIdx;

out gl_PerVertex {
    vec4 gl_Position;
};

/*
    ==========================================
main
    ==========================================
*/
void main() {
    mat4 model = modelMatrix[inModelMatrixIdx];
    // world position
    vec4 worldPos4 = model * vec4(inPosition, 1.0);
    worldPos = worldPos4.xyz;

    // correct normal matrix: transpose(inverse(mat3(model)))
    mat3 normalMat = transpose(inverse(mat3(model)));
    worldNormal = normalize(normalMat * inNormal);

    // shadow position (light-projection * light-view * worldPos)
    shadowPos = shadow.proj * shadow.view * worldPos4;

    // camera world position (use invView to get camera transform)
    camPos = (camera.invView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;

    // uv
    texCoords = inTexCoord;

    materialIdx = inMaterialIdx;

    // clip position
    gl_Position = camera.proj * camera .view * worldPos4;
}