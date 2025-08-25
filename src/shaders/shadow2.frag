#version 450

layout(binding = 3) uniform sampler2D texAlbedo;

layout(location = 0) flat in ivec2 hasTexture;
layout(location = 1) in vec2 uv;

void main() {
    float a = 1.0;
    if(hasTexture.x==0) {
        a=1.0;
    }else {
        a=texture(texAlbedo,uv).a;
    }

    if(a<0.0001) {
        discard;
    }
}