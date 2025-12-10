#ifndef WATERPLANE_H
#define WATERPLANE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <shader.h>
#include <mesh.h>

using namespace std;

class WaterPlane
{
public:
    WaterPlane(float width, float length, float height, 
               int gridResolution = 100,
               const vector<Texture>& textures = vector<Texture>())
        : m_width(width), m_length(length), m_height(height), 
          m_gridResolution(gridResolution)
    {
        std::cout << "\n=== Creating Water Plane ===" << std::endl;
        std::cout << "Size: " << width << " x " << length << std::endl;
        std::cout << "Height: " << height << std::endl;
        std::cout << "Grid resolution: " << gridResolution << std::endl;
        
        GenerateMesh(textures);
    }

    ~WaterPlane()
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

    void Update(float time)
    {
        m_time = time;
        // 可以在这里更新水面波动动画
    }

    float GetHeight() const { return m_height; }
    glm::vec3 GetCenter() const { return glm::vec3(0.0f, m_height, m_length / 2.0f); }

private:
    Mesh* m_mesh = nullptr;
    float m_width;     // X 方向宽度
    float m_length;    // Z 方向长度
    float m_height;    // Y 高度(海平面高度)
    int m_gridResolution;  // 网格细分数
    float m_time = 0.0f;

    void GenerateMesh(vector<Texture> textures)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;

        int totalVertices = (m_gridResolution + 1) * (m_gridResolution + 1);
        int totalTriangles = m_gridResolution * m_gridResolution * 2;

        std::cout << "Generating water mesh with " << totalVertices << " vertices, "
                  << totalTriangles << " triangles" << std::endl;

        vertices.reserve(totalVertices);
        indices.reserve(totalTriangles * 3);

        // 生成水面顶点网格
        float xStep = m_width / m_gridResolution;
        float zStep = m_length / m_gridResolution;

        // 水面起始位置: X轴居中,Z轴从0开始向正方向延伸
        float xStart = -m_width / 2.0f;
        float zStart = 0.0f;  // Z轴正半轴

        for (int z = 0; z <= m_gridResolution; z++) {
            for (int x = 0; x <= m_gridResolution; x++) {
                Vertex vertex;

                // 位置
                float xPos = xStart + x * xStep;
                float zPos = zStart + z * zStep;
                float yPos = m_height;  // 水平面高度
                vertex.Position = glm::vec3(xPos, yPos, zPos);

                // 纹理坐标 (平铺)
                float texScale = 10.0f;  // 纹理重复次数
                vertex.TexCoords = glm::vec2(
                    (float)x / m_gridResolution * texScale,
                    (float)z / m_gridResolution * texScale
                );

                // 法线向上
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                vertex.Bitangent = glm::vec3(0.0f, 0.0f, 1.0f);

                // 骨骼权重
                for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
                    vertex.m_BoneIDs[i] = -1;
                    vertex.m_Weights[i] = 0.0f;
                }

                vertices.push_back(vertex);
            }
        }

        // 生成索引(两个三角形组成一个四边形)
        for (int z = 0; z < m_gridResolution; z++) {
            for (int x = 0; x < m_gridResolution; x++) {
                int topLeft = z * (m_gridResolution + 1) + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * (m_gridResolution + 1) + x;
                int bottomRight = bottomLeft + 1;

                // 第一个三角形
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                // 第二个三角形
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }

        m_mesh = new Mesh(vertices, indices, textures);
        
        std::cout << "Water plane mesh created successfully!" << std::endl;
    }
};

#endif // WATERPLANE_H