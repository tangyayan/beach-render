#ifndef OCEAN_BAKED_H
#define OCEAN_BAKED_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <shader.h>
#include <vector>

class OceanBaked
{
private:
    int N;
    float Lx, Lz;
    float waterHeight;
    unsigned int VAO, VBO, EBO;
    std::vector<unsigned int> indices;
    
    unsigned int displacementTex;
    unsigned int normalTex;
    float timeSpan;

public:
    OceanBaked(int N, float Lx, float Lz, float waterHeight,
               unsigned int displacementTex, unsigned int normalTex, float timeSpan)
        : N(N), Lx(Lx), Lz(Lz), waterHeight(waterHeight),
          displacementTex(displacementTex), normalTex(normalTex), timeSpan(timeSpan)
    {
        SetupMesh();
    }
    
    ~OceanBaked()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
    
    void SetupMesh()
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> texCoords;
        
        positions.resize(N * N);
        texCoords.resize(N * N);
        
        // 生成网格位置和纹理坐标
        for (int m = 0; m < N; m++) {
            for (int n = 0; n < N; n++) {
                int index = m * N + n;
                
                // 基础位置 (x: [-Lx, Lx], z: [0, Lz])
                float x = (n / (float)(N - 1)) * 2.0f * Lx - Lx;
                float z = (m / (float)(N - 1)) * Lz;
                
                positions[index] = glm::vec3(x, waterHeight, z);
                
                // 纹理坐标 [0, 1]
                texCoords[index] = glm::vec2(n / (float)(N - 1), m / (float)(N - 1));
            }
        }
        
        // 生成索引
        for (int m = 0; m < N - 1; m++) {
            for (int n = 0; n < N - 1; n++) {
                int i0 = m * N + n;
                int i1 = m * N + n + 1;
                int i2 = (m + 1) * N + n;
                int i3 = (m + 1) * N + n + 1;
                
                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i1);
                
                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 
                     (positions.size() * sizeof(glm::vec3) + texCoords.size() * sizeof(glm::vec2)), 
                     nullptr, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 
                       positions.size() * sizeof(glm::vec3), positions.data());
        glBufferSubData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3),
                       texCoords.size() * sizeof(glm::vec2), texCoords.data());
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                     indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        // 位置
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        
        // 纹理坐标
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), 
                             (void*)(positions.size() * sizeof(glm::vec3)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
    }
    
    void Draw(Shader& shader, float time)
    {
        shader.use();
        
        // 传递时间参数(循环)
        float normalizedTime = fmod(time, timeSpan) / timeSpan;
        shader.setFloat("uTime", normalizedTime);
        
        // 绑定 3D 纹理
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_3D, displacementTex);
        shader.setInt("displacementMap", 10);
        
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_3D, normalTex);
        shader.setInt("normalMap", 11);
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    float GetHeight() const { return waterHeight; }
};

#endif // OCEAN_BAKED_H