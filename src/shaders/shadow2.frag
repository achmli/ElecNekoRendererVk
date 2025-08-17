#version 450

layout(binding = 3) uniform sampler2D texAlbedo;

layout(location = 0) in vec2 texCoord;
layout(location = 1) flat in ivec2 hasTexture;

void main() {
    float alpha = 1.0;
    if(hasTexture.x == 0) {
        alpha = 1.0;
    }else {
        alpha = texture(texAlbedo, texCoord).a;
    }

    if(alpha < 0.0001) {
        discard;
    }
}