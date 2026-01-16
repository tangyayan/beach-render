#version 330 core
out vec4 FragColor;

in vec3 Normal;

uniform vec3 lightColor;

void main()
{
    // 法线朝向视线方向（假设相机远在外面）时更亮
    vec3 N = normalize(Normal);
    float center = max(N.z, 0.0);          // 粗略假设 Z 指向观察方向
    float rim    = 1.0 - center;

    // 中心：亮，边缘：更亮一点，有一点晕开的感觉
    float intensity = 1.5 * center + 2.0 * pow(rim, 2.0);

    vec3 color = lightColor * intensity;
    FragColor = vec4(color, 1.0);
}