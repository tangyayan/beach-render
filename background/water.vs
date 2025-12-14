#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 glp;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float uTime;  // [0, 1] 循环时间

uniform sampler3D displacementMap;
uniform sampler3D normalMap;

void main()
{
    // 从 3D 纹理采样位移和法线
    vec3 uvw = vec3(aTexCoord, uTime);
    vec3 displacement = texture(displacementMap, uvw).xyz;
    vec3 normal = texture(normalMap, uvw).xyz;
    
    // 应用位移
    vec3 displacedPos = aPos + displacement;
    
    FragPos = vec3(model * vec4(displacedPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * normal;
    TexCoords = aTexCoord;
    glp = projection * view * vec4(FragPos, 1.0);
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}