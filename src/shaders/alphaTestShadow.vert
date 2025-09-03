#version 450

layout(binding = 0) uniform uboCamera {
    mat4 view;
    mat4 proj;
} camera;
layout(binding = 1) uniform uboModel {
    mat4 model;
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 uv;

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
    gl_Position = camera.proj * camera.view * model.model * vec4(inPosition, 1.0);

    uv=inTexCoord;
}