#version 450

layout(binding = 2) uniform sampler2D texAlbedo;

layout(location = 0) in vec2 uv;

void main() {
    float a = 1.0;

    a = texture(texAlbedo, uv).a;

    if(a<0.0001) {
        discard;
    }
}