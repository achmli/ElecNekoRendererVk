#version 450

layout(binding = 0) uniform uboCamera {
    mat4 view;
    mat4 proj;
} camera;
// layout(binding = 1) uniform uboModel {
//     mat4 model;
// } model;

layout(std140, binding = 1) readonly buffer uboModel {
    mat4 modelMatrix[];
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in uint inMaterialIdx;
layout(location = 4) in uint inModelMatrixIdx;

out gl_PerVertex {
    vec4 gl_Position;
};

/*
    ==========================================
main
    ==========================================
*/
void main() {
    // Project coordinate to screen
    mat4 model = modelMatrix[inModelMatrixIdx];
    // mat4 model = mat4(1.0);
    gl_Position = camera.proj * camera.view * model * vec4(inPosition, 1.0);
}