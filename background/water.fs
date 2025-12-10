#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform vec3 viewPos;
uniform Light light;
uniform sampler2D texture_diffuse1;  // 水面纹理(可选)
uniform float time;
uniform vec3 waterColor;  // 水的基础颜色

void main()
{
    // ambient
    vec3 ambient = light.ambient * waterColor;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    // vec3 lightDir = normalize(light.position - FragPos);
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * waterColor;  
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = light.specular * spec * vec3(0.5f); // assuming a constant specular color
    
    // 菲涅尔效果 (边缘更透明/反光)
    // float fresnel = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0);
    
    // vec3 result = ambient + diffuse + specular * fresnel;
    
    // 半透明效果
    // float alpha = 0.7 + fresnel * 0.3;  // 边缘更透明
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}