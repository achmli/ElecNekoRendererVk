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
} camera;
layout(binding = 1) uniform uboModel {
    mat4 model;
} model;

/*
    ==========================================
attributes
    ==========================================
*/

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

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
}