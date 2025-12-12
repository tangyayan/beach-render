#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;
uniform vec4 clippingPlane;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    
    // 启用裁剪平面
    gl_ClipDistance[0] = dot(worldPos, clippingPlane);
    
    gl_Position = projection * view * worldPos;
}