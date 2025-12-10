from PIL import Image
import math
import random

# 设置图片大小
width = 512
height = 512

# 海平面和沙滩设置
seaLevel = 200
shallowWaterWidth = 40  # 浅水区宽度
beachWidth = 80         # 沙滩宽度

# 多层波浪参数（创造更自然的海岸线）
waves = [
    {'amplitude': 30, 'frequency': 2, 'phase': 0},
    {'amplitude': 15, 'frequency': 5, 'phase': math.pi / 4},
    {'amplitude': 8, 'frequency': 10, 'phase': math.pi / 2},
]

# 创建空的灰度图
image = Image.new('L', (width, height))
pixels = image.load()

# 添加柏林噪声风格的随机性
random.seed(42)

for x in range(width):
    normalized_x = x / width
    
    # 叠加多层正弦波
    total_wave_offset = 0
    for wave in waves:
        total_wave_offset += wave['amplitude'] * math.sin(
            2 * math.pi * wave['frequency'] * normalized_x + wave['phase']
        )
        print(total_wave_offset)
    
    adjusted_seaLevel = seaLevel + total_wave_offset
    
    for y in range(height):
        if y < adjusted_seaLevel - shallowWaterWidth:
            # 深海区域 - 深度渐变
            depth = (adjusted_seaLevel - shallowWaterWidth - y) / (adjusted_seaLevel - shallowWaterWidth)
            h = int(depth * 50)  # 深海: 0-50
            
        elif y < adjusted_seaLevel:
            # 浅水区 - 从深海到海岸线的过渡
            t = (y - (adjusted_seaLevel - shallowWaterWidth)) / shallowWaterWidth
            # 使用更平滑的插值函数 (smootherstep)
            smooth_t = t * t * t * (t * (t * 6 - 15) + 10)
            h = int(50 + smooth_t * 20)  # 浅水: 50-70
            
            # 添加水下沙纹效果
            if random.random() > 0.85:
                h += random.randint(-3, 3)
                
        elif y < adjusted_seaLevel + beachWidth:
            # 沙滩区域 - 从海岸线到内陆的过渡
            t = (y - adjusted_seaLevel) / beachWidth
            
            # 使用更平滑的插值 (smootherstep)
            smooth_t = t * t * t * (t * (t * 6 - 15) + 10)
            h = int(70 + smooth_t * 130)  # 沙滩: 70-200
            
            # 添加沙丘起伏
            # 在沙滩中段添加更明显的沙丘
            if 0.3 < t < 0.7:
                dune_height = math.sin(t * math.pi * 4) * 15
                h += int(dune_height)
            
            # 随机噪声
            if random.random() > 0.75:
                noise = random.randint(-4, 4)
                h += noise
            
            h = max(70, min(200, h))
            
        else:
            # 内陆区域
            inland_distance = y - (adjusted_seaLevel + beachWidth)
            base_height = 200
            
            # 使用对数增长,让高度变化更自然
            height_increase = math.log(1 + inland_distance * 0.1) * 30
            h = min(int(base_height + height_increase), 255)
            
            # 添加地形变化
            if random.random() > 0.6:
                h = min(255, h + random.randint(0, 15))

        pixels[x, y] = h

# 可选：应用轻微的高斯模糊来进一步平滑
from PIL import ImageFilter
image = image.filter(ImageFilter.GaussianBlur(radius=0.5))

# 保存 PNG
# image.save("beach_heightmap.png")
print("生成完成：beach_heightmap.png")
print(f"使用了 {len(waves)} 层波浪叠加")
print(f"海洋深度: 0-50")
print(f"浅水区: 50-70")
print(f"沙滩: 70-200")
print(f"内陆: 200-255")