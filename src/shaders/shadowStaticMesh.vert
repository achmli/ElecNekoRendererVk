#version 450

layout(binding = 0) uniform uboCamera {
    mat4 view;
    mat4 proj;
}
camera;

struct InstanceData {
    mat4 localToWorld;
    mat4 worldToLocal;
};

layout(std430, binding = 1) readonly buffer Instances {
    InstanceData instances[];
};

layout(push_constant) uniform DrawPushConstant {
    uint materialIndex;
    uint instanceIndex;
}
pushData;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inTangent;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    InstanceData instanceData = instances[pushData.instanceIndex];
    mat4 model = instanceData.localToWorld;

    gl_Position = camera.proj * camera.view * model * vec4(inPosition, 1.0);
}
