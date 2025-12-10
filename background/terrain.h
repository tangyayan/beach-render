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
#include <iostream>
#include <shader.h>
#include <mesh.h>

using namespace std;

class Terrain
{
public:
    Terrain(const std::string& heightmapPath, float heightScale = 10.0f, float horizontalScale = 1.0f)
        : m_heightScale(heightScale), m_horizontalScale(horizontalScale)
    {
        LoadHeightmap(heightmapPath);
        GenerateMesh();
    }

    ~Terrain() = default;

    void Draw(Shader& shader)
    {
        if (m_mesh) {
            m_mesh->Draw(shader);
        }
    }

    // 添加纹理
    void AddTexture(unsigned int id, const string& type, const string& path)
    {
        Texture texture;
        texture.id = id;
        texture.type = type;
        texture.path = path;
        m_textures.push_back(texture);
        
        // 更新 mesh 的纹理
        if (m_mesh) {
            m_mesh->textures = m_textures;
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

private:
    vector<float> heightData;
    vector<Texture> m_textures;
    Mesh* m_mesh = nullptr;
    
    int m_width, m_height;
    float m_heightScale;
    float m_horizontalScale;

    void LoadHeightmap(const std::string& path)
    {
        int nrChannels;
        unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &nrChannels, 1);
        
        if (!data) {
            std::cout << "Failed to load heightmap: " << path << std::endl;
            m_width = m_height = 100;
            heightData.resize(m_width * m_height, 0.0f);
            return;
        }

        heightData.resize(m_width * m_height);
        for (int i = 0; i < m_width * m_height; i++) {
            heightData[i] = static_cast<float>(data[i]) / 255.0f;
        }
        
        stbi_image_free(data);
        std::cout << "Loaded heightmap: " << m_width << "x" << m_height << std::endl;
    }

    void GenerateMesh()
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;

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
                    static_cast<float>(x) / (m_width - 1),
                    static_cast<float>(z) / (m_height - 1)
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

        // 计算法线
        CalculateNormals(vertices, indices);
        
        // 计算切线
        CalculateTangents(vertices, indices);
        
        // 创建 Mesh 对象
        m_mesh = new Mesh(vertices, indices, m_textures);
    }

    void CalculateNormals(vector<Vertex>& vertices, const vector<unsigned int>& indices)
    {
        // 重置所有法线
        for (auto& vertex : vertices) {
            vertex.Normal = glm::vec3(0.0f);
        }

        // 累加面法线
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

        // 归一化
        for (auto& vertex : vertices) {
            vertex.Normal = glm::normalize(vertex.Normal);
        }
    }

    void CalculateTangents(vector<Vertex>& vertices, const vector<unsigned int>& indices)
    {
        // 重置切线和副切线
        for (auto& vertex : vertices) {
            vertex.Tangent = glm::vec3(0.0f);
            vertex.Bitangent = glm::vec3(0.0f);
        }

        // 计算切线和副切线
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

        // 归一化并正交化
        for (auto& vertex : vertices) {
            glm::vec3 t = vertex.Tangent;
            glm::vec3 n = vertex.Normal;
            
            // Gram-Schmidt 正交化
            t = glm::normalize(t - n * glm::dot(n, t));
            vertex.Tangent = t;
            
            // 计算 handedness
            if (glm::dot(glm::cross(n, t), vertex.Bitangent) < 0.0f) {
                t = t * -1.0f;
            }
            
            vertex.Bitangent = glm::normalize(vertex.Bitangent);
        }
    }
};

#endif // TERRAIN_H