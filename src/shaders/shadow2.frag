#version 450

layout (location = 5) in float debugMatrixIndex;
layout (location = 6) in vec3 debugWorldPos;

layout (location = 0) out vec4 outColor;

void main() {
    if (debugMatrixIndex < -1.5) {
        outColor = vec4(1.0, 1.0, 0.0, 1.0);
    } else if (debugMatrixIndex < -0.5) {
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        float hue = mod(debugMatrixIndex / 10.0, 1.0);
        outColor = vec4(hue, 0.5, 0.5, 1.0);
    }
}