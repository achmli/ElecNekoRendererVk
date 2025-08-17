#version 450
#extension GL_ARB_separate_shader_objects : enable

// layout(binding = 0) uniform uboCamera {
//     mat4 view;
//     mat4 proj;
// } camera;

// layout(location = 0) in vec3 inPosition;

// layout(location = 0) out vec3 viewDir;

// out gl_PerVertex {
//     vec4 gl_Position;
// };

void main() {
    // viewDir = mat3(camera.view) * inPosition;

    // gl_Position = camera.proj * vec4(viewDir, 1.0);
    vec2 pos;
    if(gl_VertexIndex == 0) {
        pos = vec2(-1.0, -1.0);
    }else if(gl_VertexIndex == 1) {
        pos = vec2(3.0, -1.0);
    }else {
        pos = vec2(-1.0, 3.0);
    }

    gl_Position = vec4(pos, 0.0, 1.0);
}