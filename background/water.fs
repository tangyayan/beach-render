#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 glp;  // ClipSpace position

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
uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform sampler2D depthTexture;
uniform float time;
uniform vec3 waterColor;
uniform int isAbove;
uniform float shininess;

void main()
{
    vec2 ndc = glp.xy / glp.w;
    vec2 refractTexCoords = ndc * 0.5 + 0.5;
    vec2 reflectTexCoords = vec2(refractTexCoords.x, 1.0 - refractTexCoords.y);
    
    refractTexCoords = clamp(refractTexCoords, 0.001, 0.999);
    reflectTexCoords = clamp(reflectTexCoords, 0.001, 0.999);
    
    vec4 refractColor = texture(refractionTexture, refractTexCoords);
    vec4 reflectColor = texture(reflectionTexture, reflectTexCoords);
    
    // 计算光照
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 方向光
    vec3 lightDir = normalize(-dirLight.direction);
    
    // 漫反射
    float diff = max(dot(norm, lightDir), 0.0);
    
    // 镜面反射
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    vec3 ambient = dirLight.ambient * waterColor;
    vec3 diffuse = dirLight.diffuse * diff * waterColor;
    vec3 specular = dirLight.specular * spec * vec3(0.5);
    
    vec4 blueColor = vec4(0.0, 0.25, 1.0, 1.0);
    float cosFres = dot(viewDir, norm);
    
    if (isAbove == 1) {
        // 水面上方：混合反射和折射
        float fresnel = 1.0 - cosFres;
        fresnel = pow(clamp(fresnel, 0.0, 1.0), 3.0);
        
        reflectColor.rgb = reflectColor.rgb * (ambient + diffuse) + 0.75 * specular;
        // FragColor = reflectColor * fresnel;
        FragColor = vec4(ambient + diffuse + specular, 1.0);
        // FragColor = mix(vec4(waterColor * result, 1.0), reflectColor, fresnel);
    } else {
        // 水面下方：蓝色调 + 折射
        vec3 result = ambient + diffuse + specular;
        vec4 mixColor = 0.5 * blueColor + 0.6 * refractColor;
        FragColor = vec4(mixColor.rgb * result, 1.0);
    }
}