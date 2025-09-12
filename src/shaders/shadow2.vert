#version 450

// 在顶点着色器中添加调试输出
layout(binding = 0) uniform uboCamera {
    mat4 view;
    mat4 proj;
} camera;

layout(std140, binding = 1) readonly buffer uboModel {
    mat4 modelMatrix[];
};

layout(binding = 2) uniform uboShadow {
    mat4 view;
    mat4 proj;
} shadow;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in int inMaterialIdx;
layout(location = 4) in int inModelMatrixIdx;

// 添加调试输出
layout (location = 5) out float debugMatrixIndex;
layout (location = 6) out vec3 debugWorldPos;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    debugMatrixIndex = float(inModelMatrixIdx);

    if (inModelMatrixIdx < 0 || inModelMatrixIdx >= modelMatrix.length()) {
        gl_Position = camera.proj * camera.view * vec4(inPosition, 1.0);
        debugWorldPos = inPosition;
    } else {
        mat4 model = modelMatrix[inModelMatrixIdx];

        bool matrixIsValid = false;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (model[i][j] != 0.0) {
                    matrixIsValid = true;
                    break;
                }
            }
            if (matrixIsValid) break;
        }

        if (!matrixIsValid) {
            gl_Position = camera.proj * camera.view * vec4(inPosition, 1.0);
            debugWorldPos = inPosition;

            debugMatrixIndex = -2.0;
        } else {
            vec4 worldPos = model * vec4(inPosition, 1.0);
            gl_Position = camera.proj * camera.view * worldPos;
            debugWorldPos = worldPos.xyz;

            debugMatrixIndex = float(inModelMatrixIdx);
        }
    }
}