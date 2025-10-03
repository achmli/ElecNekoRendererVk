#version 450

layout(binding = 0) uniform uboView {
    mat4 view;
} uView;

layout(binding = 1) uniform uboProj {
    mat4 proj;
} uProj;

layout(std140, binding = 2) readonly buffer uboModel {
    mat4 modelMatrix[];
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in uint inMaterialIdx;
layout(location = 4) in uint inModelMatrixIdx;

layout(location = 0) out vec2 uv;
layout(location = 1) flat out uint materialIdx;

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

    gl_Position = uProj.proj * uView.view * model * vec4(inPosition, 1.0);

    uv = inTexCoord;
    materialIdx = inMaterialIdx;
}