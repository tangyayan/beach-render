#version 330 core
out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube skybox;
uniform int isAbove;

// 0 = 最深夜, 1 = 正午
uniform float m_dayFactor;

// 简单 gamma 校正函数（可选）
vec3 applyGamma(vec3 color, float gamma)
{
    return pow(color, vec3(1.0 / gamma));
}

void main()
{
    vec4 baseColor = texture(skybox, TexCoords);

    // 原有水下逻辑保留
    if (isAbove == 0 && TexCoords.y < 0.47)
    {
        FragColor = vec4(0.0, 0.0, 0.2, 1.0);
        return;
    }

    float t = clamp(m_dayFactor, 0.0, 1.0);

    // ---------------------------
    // 1. 定义几个关键时刻的“天空基色”
    // ---------------------------

    // 深夜（t≈0 或 t≈1）
    vec3 nightColor = vec3(0.02, 0.04, 0.10);

    // 日出前（天稍亮、带一点紫）
    vec3 dawnColor  = vec3(0.15, 0.10, 0.25);

    // 日出瞬间（橙红）
    vec3 sunriseColor = vec3(0.9, 0.4, 0.2);

    // 白天（稍微偏暖）
    vec3 dayColor = vec3(1.0, 1.0, 1.0);

    // 黄昏（类似日出，但更暗）
    vec3 sunsetColor = vec3(0.9, 0.35, 0.15);

    // 深夜 fallback 用 nightColor

    // ---------------------------
    // 2. 根据 t 所在区间做多段插值
    //
    // 约定：
    //   [0.00, 0.20): 深夜 -> 日出前
    //   [0.20, 0.35): 日出前 -> 日出
    //   [0.35, 0.65): 白天过渡
    //   [0.65, 0.80): 白天 -> 黄昏
    //   [0.80, 1.00]: 黄昏 -> 深夜
    // ---------------------------

    vec3 skyTint;
    if (t < 0.20)
    {
        // 深夜 -> 日出前
        float k = smoothstep(0.00, 0.20, t);
        skyTint = mix(nightColor, dawnColor, k);
    }
    else if (t < 0.35)
    {
        // 日出前 -> 日出
        float k = smoothstep(0.20, 0.35, t);
        skyTint = mix(dawnColor, sunriseColor, k);
    }
    else if (t < 0.65)
    {
        // 日出 -> 白天
        float k = smoothstep(0.35, 0.65, t);
        skyTint = mix(sunriseColor, dayColor, k);
    }
    else if (t < 0.80)
    {
        // 白天 -> 黄昏
        float k = smoothstep(0.65, 0.80, t);
        skyTint = mix(dayColor, sunsetColor, k);
    }
    else
    {
        // 黄昏 -> 深夜
        float k = smoothstep(0.80, 1.00, t);
        skyTint = mix(sunsetColor, nightColor, k);
    }

    // ---------------------------
    // 3. 叠加在原始 cubemap 上
    //    可用乘法（调色）+ 插值（保留细节）
    // ---------------------------

    // 贴图颜色做个轻微 gamma 矫正，让高光更自然（可选）
    vec3 texCol = applyGamma(baseColor.rgb, 2.2);

    // 将 skyTint 看作“环境色”，用 mix 控制其影响程度
    // t 越接近白天，越多保留原始贴图；越接近夜晚，越强烈上色。
    float tintStrengthNight = 0.8; // 夜晚对贴图影响更大
    float tintStrengthDay   = 0.3; // 白天则弱一些
    float tintStrength = mix(tintStrengthNight, tintStrengthDay, t);

    vec3 tinted = mix(texCol, skyTint, tintStrength);

    // 再做一个整体亮度控制
    float nightBrightness = 0.3;
    float dayBrightness   = 1.0;
    float brightness      = mix(nightBrightness, dayBrightness, t);

    vec3 finalColor = tinted * brightness;

    // 再次 gamma 回来
    finalColor = applyGamma(finalColor, 1.0 / 2.2);

    FragColor = vec4(finalColor, 1.0);
}