#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) out vec3 vTexCoords;

layout(binding = 0) uniform uboCamera {
    mat4 view;
    mat4 proj;
    mat4 viewNoTrans;
} camera;

vec3 positions[8]=vec3[8](
    vec3(-1.0, -1.0, -1.0),
    vec3(1.0, -1.0, -1.0),
    vec3(1.0,  1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),
    vec3(-1.0, -1.0,  1.0),
    vec3(1.0, -1.0,  1.0),
    vec3(1.0,  1.0,  1.0),
    vec3(-1.0,  1.0,  1.0));

void main() {
    int idxs[36]=int[36](
        0,1,2,  2,3,0,    // -Z 
        4,5,6,  6,7,4,    // +Z 
        0,4,7,  7,3,0,    // -X 
        1,5,6,  6,2,1,    // +X 
        0,1,5,  5,4,0,    // -Y 
        3,2,6,  6,7,3);     // +Y

    vec3 pos = positions[idxs[gl_VertexIndex]];

    vTexCoords = pos;

    vec4 clipPos = camera.proj * camera.viewNoTrans * vec4(pos, 1.0);

    gl_Position = clipPos;
    // gl_Position.z = gl_Position.w;
}