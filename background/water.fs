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
uniform int isAbove;

uniform float shininess;

void main()
{
    vec3 norm = normalize(Normal);

    vec2 ndc = glp.xy / glp.w;
    vec2 refractTexCoords = ndc * 0.5 + 0.5 + norm.xz * 0.02;
    vec2 reflectTexCoords = vec2(refractTexCoords.x, 1.0 - refractTexCoords.y);
    
    refractTexCoords = clamp(refractTexCoords, 0.001, 0.999);
    reflectTexCoords = clamp(reflectTexCoords, 0.001, 0.999);
    
    vec4 refractColor = texture(refractionTexture, refractTexCoords);
    vec4 reflectColor = texture(reflectionTexture, reflectTexCoords);

    float depthValue = texture(depthTexture, refractTexCoords).r;
    // 从深度纹理重建视空间深度
    float near = 0.1;   // 需要与你的相机近平面一致
    float far = 1000.0; // 需要与你的相机远平面一致
    // 地面深度(折射纹理对应的深度)
    float floorDepth = (2.0 * near * far) / (far + near - (2.0 * depthValue - 1.0) * (far - near));
    // 水面深度(当前片段的深度)
    float waterDepth = (2.0 * near * far) / (far + near - (2.0 * gl_FragCoord.z - 1.0) * (far - near));
    // 水的深度差
    float depth = floorDepth - waterDepth;
    depth = clamp(depth, 0.0, 50.0); // 限制最大深度

    // 计算光照
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 方向光
    vec3 lightDir = normalize(-dirLight.direction);
    
    // 漫反射
    float diff = max(dot(norm, lightDir), 0.0);
    
    // 镜面反射
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    vec3 ambient = dirLight.ambient * 0.3;
    vec3 diffuse = dirLight.diffuse * diff * 0.5;
    vec3 specular = dirLight.specular * spec * vec3(1);
    
    vec4 blueColor = vec4(0.0, 0.25, 1.0, 1.0);
    float cosFres = dot(viewDir, norm);
    
    if (isAbove == 1) {
        // 水面上方
        float fresnel = 1.0 - cosFres;
        fresnel = pow(clamp(fresnel, 0.0, 1.0), 3.0);
        
        // 1. 定义深度相关的颜色
        vec3 shallowColor = vec3(0.1, 0.6, 0.8);  // 浅水:浅蓝绿色
        vec3 deepColor = vec3(0.0, 0.1, 0.3);     // 深水:深蓝色
        
        // 2. 根据深度混合颜色
        float depthFactor = 1.0 - exp(-depth * 0.05);  // 指数衰减
        vec3 waterDepthColor = mix(shallowColor, deepColor, depthFactor);
        // 3. 计算透明度(深度越深越不透明)
        float waterAlpha = clamp(depth * 0.03, 0.4, 0.95);  // 0.4~0.95
        // 4. 混合折射和深度颜色
        vec3 refraction = mix(refractColor.rgb, waterDepthColor, depthFactor * 0.7);
        
        // 5. 应用光照到深度颜色
        vec3 litWaterColor = waterDepthColor * (ambient + diffuse) + specular;
        
        // 6. 使用 Fresnel 混合反射和折射
        vec3 reflection = reflectColor.rgb * (0.8 + 0.2 * (ambient + diffuse)) + specular * 0.5;
        
        // 7. 最终颜色混合
        vec3 finalColor = mix(refraction, reflection, fresnel * 0.6);
        finalColor = mix(finalColor, litWaterColor, 0.3);
        
        // 8. 输出带透明度的颜色
        FragColor = vec4(finalColor, waterAlpha);

        // reflectColor.rgb = reflectColor.rgb * (0.8 + 0.2*(ambient + diffuse)) + 0.5 * specular;
        // FragColor = vec4(ambient + diffuse + specular, 1.0);
        // FragColor = mix(vec4(waterColor, 1.0), reflectColor, fresnel);
    } else {
        // 水面下方：蓝色调 + 折射
        // FragColor = vec4(finalColor, 1.0);
        vec4 mixColor = (0.5 * blueColor) + (0.6 * refractColor);
		FragColor = vec4((mixColor.xyz * (ambient + diffuse) + 0.75*specular), 1.0);
    }
}