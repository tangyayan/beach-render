#ifndef OCEAN_FFT_BAKER_H
#define OCEAN_FFT_BAKER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include "waterplane_gerstner.h"

class OceanFFTBaker
{
private:
    int N;              // 空间分辨率
    int T;              // 时间帧数
    float timeSpan;     // 时间跨度(秒)
    
    unsigned int texture3D_displacement;  // 3D 位移纹理 (xyz)
    unsigned int texture3D_normal;        // 3D 法线纹理
    
    OceanGerstnerFFT* ocean;

public:
    OceanFFTBaker(int N, int T, float timeSpan,
                  float Lx, float Lz, 
                  float A = 0.0005f,
                  glm::vec2 windDir = glm::vec2(1.0f, 0.5f), 
                  float windSpeed = 30.0f)
        : N(N), T(T), timeSpan(timeSpan)
    {
        std::cout << "\n=== Starting FFT Baking ===" << std::endl;
        std::cout << "Spatial Resolution: " << N << "x" << N << std::endl;
        std::cout << "Time Frames: " << T << std::endl;
        std::cout << "Time Span: " << timeSpan << "s" << std::endl;
        std::cout << "Ocean Size: " << Lx << " x " << Lz << std::endl;
        
        // 创建临时 FFT 对象用于烘焙
        ocean = new OceanGerstnerFFT(N, Lx, Lz, A, windDir, windSpeed);
    
        // 计算最小波长对应的最大频率
        float minWavelength = std::min(2.0f * Lx / N, 2.0f * Lz / N);
        float k_max = 2.0f * M_PI / minWavelength;
        float omega_max = std::sqrt(9.81f * k_max);
        float minPeriod = 2.0f * M_PI / omega_max;
        
        // 让 timeSpan 是最小周期的整数倍
        int numPeriods = std::max(1, (int)(timeSpan / minPeriod));
        float adjustedTimeSpan = numPeriods * minPeriod;
        
        if (std::abs(adjustedTimeSpan - timeSpan) > 0.1f) {
            std::cout << "Adjusting timeSpan from " << timeSpan 
                      << "s to " << adjustedTimeSpan << "s for seamless loop" << std::endl;
            timeSpan = adjustedTimeSpan;
        }
        
        this->timeSpan = timeSpan;
        
        BakeTextures();
        
        delete ocean;
        ocean = nullptr;
        
        std::cout << "FFT Baking Complete!" << std::endl;
    }
    
    ~OceanFFTBaker()
    {
        glDeleteTextures(1, &texture3D_displacement);
        glDeleteTextures(1, &texture3D_normal);
    }
    
    // 烘焙 3D 纹理
    void BakeTextures()
    {
        // 准备数据缓冲 (N x N x T)
        std::vector<glm::vec3> displacementData(N * N * T);
        std::vector<glm::vec3> normalData(N * N * T);
        
        std::cout << "\nBaking frames:" << std::endl;
        
        // 对每个时间步进行 FFT 计算
        for (int t = 0; t < T; t++) {
            float time = (t / (float)(T - 1)) * timeSpan;
            
            // 进度条
            float progress = (t + 1) / (float)T * 100.0f;
            std::cout << "\rProgress: [";
            int barWidth = 50;
            int pos = barWidth * progress / 100.0f;
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << int(progress) << "% (t=" << time << "s)" << std::flush;
            
            // 计算这一帧的波浪
            ocean->Update(time);
            
            // 获取位移和法线数据
            const auto& vertices = ocean->GetVertices();
            const auto& originalPos = ocean->GetOriginalPositions();
            const auto& normals = ocean->GetNormals();
            
            // 存储到 3D 纹理缓冲
            for (int m = 0; m < N; m++) {
                for (int n = 0; n < N; n++) {
                    int spatialIdx = m * N + n;
                    int volumeIdx = t * N * N + m * N + n;
                    
                    // 存储位移量 (不是绝对位置)
                    displacementData[volumeIdx] = vertices[spatialIdx] - originalPos[spatialIdx];
                    normalData[volumeIdx] = normals[spatialIdx];
                }
            }
        }

        ocean->DebugOutput(0.0f); // 输出调试信息
        ocean->DebugOutput(timeSpan / 2.0f);
        ocean->DebugOutput(timeSpan - 0.01f);
        
        std::cout << "\n\nUploading to GPU..." << std::endl;
        
        // 创建 3D 位移纹理
        glGenTextures(1, &texture3D_displacement);
        glBindTexture(GL_TEXTURE_3D, texture3D_displacement);
        
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, 
                     N, N, T, 0, 
                     GL_RGB, GL_FLOAT, displacementData.data());
        
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        
        // ✅ 创建 3D 法线纹理
        glGenTextures(1, &texture3D_normal);
        glBindTexture(GL_TEXTURE_3D, texture3D_normal);
        
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, 
                     N, N, T, 0, 
                     GL_RGB, GL_FLOAT, normalData.data());
        
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        
        // 计算内存占用
        float memoryMB = (N * N * T * 3 * sizeof(float) * 2) / (1024.0f * 1024.0f);
        std::cout << "GPU Memory: " << memoryMB << " MB" << std::endl;
        std::cout << "Displacement Texture ID: " << texture3D_displacement << std::endl;
        std::cout << "Normal Texture ID: " << texture3D_normal << std::endl;
    }
    
    unsigned int GetDisplacementTexture() const { return texture3D_displacement; }
    unsigned int GetNormalTexture() const { return texture3D_normal; }
    float GetTimeSpan() const { return timeSpan; }
    int GetResolution() const { return N; }
};

#endif // OCEAN_FFT_BAKER_H