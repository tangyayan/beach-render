#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 glp;  // ClipSpace position

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform vec3 viewPos;
uniform vec3 cameraPos;
uniform Light light;
uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;
uniform sampler2D depthTexture;
uniform float time;
uniform vec3 waterColor;
uniform int isAbove;
uniform float shininess;

void main()
{
    vec2 ndc = glp.xy / glp.w;                    // 透视除法: [-w, w] -> [-1, 1]
    vec2 refractTexCoords = ndc * 0.5 + 0.5;     // 转换到 [0, 1]
    vec2 reflectTexCoords = vec2(refractTexCoords.x, 1.0 - refractTexCoords.y);  // Y轴翻转
    
    refractTexCoords = clamp(refractTexCoords, 0.001, 0.999);
    reflectTexCoords = clamp(reflectTexCoords, 0.001, 0.999);
    
    vec4 refractColor = texture(refractionTexture, refractTexCoords);
    vec4 reflectColor = texture(reflectionTexture, reflectTexCoords);
    
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
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular * spec * vec3(0.5f);
    
    vec4 blueColor = vec4(0.0, 0.25, 1.0, 1.0);
    float cosFres = dot(viewDir, norm);
    
    if (isAbove==1) {
        // 水面上方：混合反射和折射
        float fresnel = acos(cosFres);
        fresnel = pow(clamp(fresnel-0.3, 0.0, 1.0), 3.0);
        reflectColor = vec4((reflectColor.xyz * (ambient + diffuse) + 0.75*specular), 1.0);
        FragColor = mix(vec4(waterColor, 1.0), reflectColor, fresnel);
    } else {
        // 水面下方：蓝色调 + 折射
        vec4 mixColor = 0.5 * blueColor + 0.6 * refractColor;
        FragColor = vec4(mixColor.rgb * (ambient + diffuse) + specular * 0.75, 1.0);
    }
}