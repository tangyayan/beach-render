#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
}; 

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  

#define NR_POINT_LIGHTS 4

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform sampler2D texture_diffuse1;
uniform sampler2D shadowMap;
uniform float shininess;
uniform int isAbove;

// 阴影计算函数（带 PCF 软阴影）
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // 透视除法
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // 转换到 [0,1] 范围
    projCoords = projCoords * 0.5 + 0.5;
    
    // 超出阴影贴图范围时不产生阴影
    if(projCoords.z > 1.0)
        return 0.0;
    
    // 获取最近点的深度（从光的角度）
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    
    // 获取当前片段深度
    float currentDepth = projCoords.z;
    
    // 动态偏移（防止阴影失真/痤疮 shadow acne）
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF（Percentage Closer Filtering）软阴影
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    // 3x3 采样核心
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // 边界柔化（阴影边缘渐变）
    if(projCoords.x < 0.0 || projCoords.x > 1.0 || 
       projCoords.y < 0.0 || projCoords.y > 1.0)
        shadow = 0.0;
    
    return shadow;
}

// 修改：定向光计算（添加阴影）
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, float shadow)
{
    vec3 lightDir = normalize(-light.direction);
    
    // 漫反射着色
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 镜面光着色
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    // 合并结果
    vec3 ambient  = light.ambient  * texture(texture_diffuse1, TexCoords).rgb;
    vec3 diffuse  = light.diffuse  * diff * texture(texture_diffuse1, TexCoords).rgb;
    vec3 specular = light.specular * spec * vec3(0.5);
    
    // 阴影只影响漫反射和镜面反射，不影响环境光
    return ambient + (1.0 - shadow) * (diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // 漫反射着色
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 镜面光着色
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    // 衰减
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                               light.quadratic * (distance * distance));    
    
    // 合并结果
    vec3 ambient  = light.ambient  * texture(texture_diffuse1, TexCoords).rgb;
    vec3 diffuse  = light.diffuse  * diff * texture(texture_diffuse1, TexCoords).rgb;
    vec3 specular = light.specular * spec * vec3(0.5);
    
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    
    return (ambient + diffuse + specular);
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 计算定向光的阴影
    vec3 lightDir = normalize(-dirLight.direction);
    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);
    
    // Phase 1: 定向光（带阴影）
    vec3 result = CalcDirLight(dirLight, norm, viewDir, shadow);
    
    // Phase 2: 点光源（暂不支持阴影，可以添加）
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    
    FragColor = vec4(result, 1.0);
    
    // 雾效（水下）
    if(isAbove == 0)
    {
        vec4 fogColor = vec4(0.0, 0.0, 0.2, 1.0);
        float fogStart = 10.0;
        float fogEnd = 200.0;
        float dist = length(viewPos - FragPos);
        float fogFactor = clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);
        FragColor = mix(fogColor, FragColor, fogFactor);
    }
}