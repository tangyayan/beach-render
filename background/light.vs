#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;
uniform vec4 clippingPlane;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_ClipDistance[0] = dot(worldPos, clippingPlane);

    // 法线：模型空间位置归一化
    Normal = mat3(transpose(inverse(model))) * aPos;

    gl_Position = projection * view * worldPos;
}