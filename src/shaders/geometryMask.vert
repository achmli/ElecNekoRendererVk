#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform uboCamera {
    mat4 view;
    mat4 proj;
    mat4 viewNoTrans;
    mat4 invView;
    mat4 invProj;
} camera;

layout(std140, binding = 1) readonly buffer uboModel {
    mat4 modelMatrix[];
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in uint inMaterialIdx;
layout(location = 4) in uint inModelMatrixIdx;

layout(location = 0) out vec3 worldNormal;
layout(location = 1) out vec2 texCoords;
layout(location = 2) flat out uint materialIdx;

void main(){
    mat4 model;
    model = modelMatrix[inModelMatrixIdx];

    mat3 normalMat=transpose(inverse(mat3(model)));
    worldNormal=normalize(normalMat * inNormal);

    texCoords = inTexCoord;
    materialIdx = inMaterialIdx;

    gl_Position = camera.proj * camera.view * model * vec4(inPosition, 1.0);
}