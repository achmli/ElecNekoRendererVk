#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform uboCamera {
    mat4 view;
    mat4 proj;
    mat4 viewNoTrans;
    mat4 invView;
    mat4 invProj;
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

layout(location = 0) out vec3 worldNormal;
layout(location = 1) out vec2 texCoords;
layout(location = 2) flat out uint materialIdx;
layout(location = 3) out vec3 viewPos;

void main() {
    InstanceData instanceData = instances[pushData.instanceIndex];

    mat4 model = instanceData.localToWorld;

    mat3 normalMat = transpose(inverse(mat3(model)));
    worldNormal = normalize(normalMat * inNormal);

    texCoords = inTexCoord;
    materialIdx = pushData.materialIndex;

    vec4 viewPos4 = camera.view * model * vec4(inPosition, 1.0);
    viewPos = viewPos4.xyz;

    gl_Position = camera.proj * viewPos4;
}
