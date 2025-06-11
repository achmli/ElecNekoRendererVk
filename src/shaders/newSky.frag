#version 450

layout(location=0) in vec3 vTexCoords;

layout(location = 0) out vec4 outColor;

layout(binding=1) uniform samplerCube skyboxSampler;

void main() {
    vec3 dir=normalize(vTexCoords);
    vec3 color=texture(skyboxSampler,dir).rgb;

    outColor=vec4(color,1.0);
}