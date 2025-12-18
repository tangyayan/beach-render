#ifndef TERRAIN_H
#define TERRAIN_H

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <shader.h>
#include <mesh.h>
#define PI 3.14159265359f

using namespace std;

class Terrain
{
public:
    Terrain(const std::string& heightmapPath, const vector<Texture>& textures, 
            float heightScale = 10.0f, float horizontalScale = 1.0f,
            int lodLevel = 1,// 添加 LOD 级别参数
            float deepwaterHeight = -1.0f,
            float maxDistance = 0.8f
        )  
        : m_heightScale(heightScale), m_horizontalScale(horizontalScale), m_lodLevel(lodLevel),
        m_deepwaterHeight(deepwaterHeight), maxDistance(maxDistance)
    {
        LoadHeightmap(heightmapPath);
        GenerateMesh(textures);
    }

    ~Terrain() 
    {
        if (m_mesh) {
            delete m_mesh;
            m_mesh = nullptr;
        }
    }

    void Draw(Shader& shader)
    {
        if (m_mesh) {
            m_mesh->Draw(shader);
        }
    }

    float GetHeight(float x, float z) const
    {
        int col = static_cast<int>((x / m_horizontalScale) + m_width / 2.0f);
        int row = static_cast<int>((z / m_horizontalScale) + m_height / 2.0f);
        
        if (col >= 0 && col < m_width && row >= 0 && row < m_height) {
            return heightData[row * m_width + col] * m_heightScale;
        }
        return 0.0f;
    }

    // 获取地形信息
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    glm::vec3 GetCenter() const { return glm::vec3(0.0f, 0.0f, 0.0f); }
    float GetMaxHeight() const 
    {
        float maxH = 0.0f;
        for (float h : heightData) {
            if (h > maxH) maxH = h;
        }
        return maxH * m_heightScale;
    }

private:
    vector<float> heightData;
    Mesh* m_mesh = nullptr;
    
    int m_width, m_height;
    int m_lodLevel;  // LOD 级别 (1=全分辨率, 2=半分辨率, 4=1/4分辨率)
    float m_heightScale;
    float m_horizontalScale;
    float m_deepwaterHeight;
    float maxDistance;

    void LoadHeightmap(const std::string& path)
    {
        int nrChannels;
        unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &nrChannels, 1);
        
        if (!data) {
            std::cout << "Failed to load heightmap: " << path << std::endl;
            std::cout << "Error: " << stbi_failure_reason() << std::endl;
            m_width = m_height = 100;
            heightData.resize(m_width * m_height, 0.0f);
            return;
        }

        std::cout << "Loaded heightmap: " << m_width << "x" << m_height 
                  << " (" << nrChannels << " channels)" << std::endl;

        // 降采样大型高度图
        if (m_lodLevel > 1) {
            int newWidth = m_width / m_lodLevel;
            int newHeight = m_height / m_lodLevel;
            
            heightData.resize(newWidth * newHeight);
            
            for (int z = 0; z < newHeight; z++) {
                for (int x = 0; x < newWidth; x++) {
                    int srcX = x * m_lodLevel;
                    int srcZ = z * m_lodLevel;
                    int srcIdx = srcZ * m_width + srcX;
                    int dstIdx = z * newWidth + x;
                    
                    heightData[dstIdx] = static_cast<float>(data[srcIdx]) / 255.0f;
                }
            }
            
            m_width = newWidth;
            m_height = newHeight;
            
            std::cout << "Downsampled to: " << m_width << "x" << m_height 
                      << " (LOD " << m_lodLevel << ")" << std::endl;
        } else {
            heightData.resize(m_width * m_height);
            for (int i = 0; i < m_width * m_height; i++) {
                heightData[i] = static_cast<float>(data[i]) / 255.0f;
            }
        }
        
        stbi_image_free(data);

        for(int x=0;x<m_width;x++){
            for(int z=m_height/2+3;z<m_height;z++){
                int idx = z * m_width + x;
                int stard_idx = (m_height/2) * m_width + x;
                float t = std::clamp((z-m_height/2.0f) / maxDistance, 0.0f, 1.0f);
                t = t*t*(3-2*t); // smoothstep
                heightData[idx] = heightData[idx]*0.2 + heightData[stard_idx] + (m_deepwaterHeight - heightData[stard_idx]) * t * 0.8;
                heightData[idx] *= 0.5f;
                // printf("x:%d z:%d h:%.3f\n", x, z, heightData[idx]);
            }
        }
        
        float minH = 1.0f, maxH = 0.0f;
        for (float h : heightData) {
            if (h < minH) minH = h;
            if (h > maxH) maxH = h;
        }
        std::cout << "Height range: " << minH << " to " << maxH << std::endl;
    }

    void GenerateMesh(vector<Texture> m_textures)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;

        int totalVertices = m_width * m_height;
        int totalTriangles = (m_width - 1) * (m_height - 1) * 2;
        
        std::cout << "Generating mesh with " << totalVertices << " vertices, " 
                  << totalTriangles << " triangles" << std::endl;

        vertices.reserve(totalVertices);
        indices.reserve(totalTriangles * 3);

        // 生成顶点
        for (int z = 0; z < m_height; z++) {
            for (int x = 0; x < m_width; x++) {
                Vertex vertex;
                
                // 位置
                float xPos = (x - m_width / 2.0f) * m_horizontalScale;
                float zPos = (z - m_height / 2.0f) * m_horizontalScale;
                float yPos = heightData[z * m_width + x] * m_heightScale;
                vertex.Position = glm::vec3(xPos, yPos, zPos);
                
                // 纹理坐标
                vertex.TexCoords = glm::vec2(
                    static_cast<float>(x) / (m_width - 1) * 10,
                    static_cast<float>(z) / (m_height - 1) * 10
                );
                
                // 初始化法线和切线
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                vertex.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
                
                // 初始化骨骼权重
                for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
                    vertex.m_BoneIDs[i] = -1;
                    vertex.m_Weights[i] = 0.0f;
                }
                
                vertices.push_back(vertex);
            }
        }

        // 生成索引
        for (int z = 0; z < m_height - 1; z++) {
            for (int x = 0; x < m_width - 1; x++) {
                int topLeft = z * m_width + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * m_width + x;
                int bottomRight = bottomLeft + 1;

                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }

        std::cout << "Calculating normals..." << std::endl;
        CalculateNormals(vertices, indices);
        
        std::cout << "Calculating tangents..." << std::endl;
        CalculateTangents(vertices, indices);
        
        std::cout << "Creating mesh..." << std::endl;
        m_mesh = new Mesh(vertices, indices, m_textures);
        
        std::cout << "Terrain mesh created successfully!" << std::endl;
    }

    void CalculateNormals(vector<Vertex>& vertices, const vector<unsigned int>& indices)
    {
        for (auto& vertex : vertices) {
            vertex.Normal = glm::vec3(0.0f);
        }

        for (size_t i = 0; i < indices.size(); i += 3) {
            unsigned int idx0 = indices[i];
            unsigned int idx1 = indices[i + 1];
            unsigned int idx2 = indices[i + 2];

            glm::vec3 v0 = vertices[idx0].Position;
            glm::vec3 v1 = vertices[idx1].Position;
            glm::vec3 v2 = vertices[idx2].Position;

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            vertices[idx0].Normal += normal;
            vertices[idx1].Normal += normal;
            vertices[idx2].Normal += normal;
        }

        for (auto& vertex : vertices) {
            vertex.Normal = glm::normalize(vertex.Normal);
        }
    }

    void CalculateTangents(vector<Vertex>& vertices, const vector<unsigned int>& indices)
    {
        for (auto& vertex : vertices) {
            vertex.Tangent = glm::vec3(0.0f);
            vertex.Bitangent = glm::vec3(0.0f);
        }

        for (size_t i = 0; i < indices.size(); i += 3) {
            unsigned int idx0 = indices[i];
            unsigned int idx1 = indices[i + 1];
            unsigned int idx2 = indices[i + 2];

            glm::vec3 pos0 = vertices[idx0].Position;
            glm::vec3 pos1 = vertices[idx1].Position;
            glm::vec3 pos2 = vertices[idx2].Position;

            glm::vec2 uv0 = vertices[idx0].TexCoords;
            glm::vec2 uv1 = vertices[idx1].TexCoords;
            glm::vec2 uv2 = vertices[idx2].TexCoords;

            glm::vec3 edge1 = pos1 - pos0;
            glm::vec3 edge2 = pos2 - pos0;
            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y + 0.0001f);

            glm::vec3 tangent;
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

            glm::vec3 bitangent;
            bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
            bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
            bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

            vertices[idx0].Tangent += tangent;
            vertices[idx1].Tangent += tangent;
            vertices[idx2].Tangent += tangent;

            vertices[idx0].Bitangent += bitangent;
            vertices[idx1].Bitangent += bitangent;
            vertices[idx2].Bitangent += bitangent;
        }

        for (auto& vertex : vertices) {
            glm::vec3 t = vertex.Tangent;
            glm::vec3 n = vertex.Normal;
            
            t = glm::normalize(t - n * glm::dot(n, t));
            vertex.Tangent = t;
            
            if (glm::dot(glm::cross(n, t), vertex.Bitangent) < 0.0f) {
                t = t * -1.0f;
            }
            
            vertex.Bitangent = glm::normalize(vertex.Bitangent);
        }
    }
};

#endif // TERRAIN_H