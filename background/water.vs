#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

void main()
{
    // 添加简单的波浪动画
    vec3 pos = aPos;
    
    //float wave1 = sin(pos.x * 1.5 + time * 2.0) * 10.0f;
    //float wave2 = sin(pos.x * 2.3 + time * 1.5) * 5.0f;
    //float wave3 = sin(pos.x * 3.4 + time * 1.8) * 1.0f;
    //pos.z += wave1 + wave2 + wave3;
    
    FragPos = vec3(model * vec4(pos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}