#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <myinclude/mesh.h>
#include <myinclude/shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
using namespace std;

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model 
{
public:
    static std::unordered_map<std::string, Model*> modelList; // 储存所有模型，供物体使用

    // model data 
    vector<Texture> textures_loaded;
    vector<Mesh>    meshes;
    string directory;
    bool flipY;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool flipY = false) : flipY(flipY), aabb_min(glm::vec3(FLT_MAX)), aabb_max(glm::vec3(-FLT_MAX))
    {
        loadModel(path);
        modelList[path] = this; // 将模型存入静态列表
    }

    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    // 获取模型的原始包围盒（未缩放）
    void GetBoundingBox(glm::vec3& min, glm::vec3& max) const
    {
        if(aabb_min.x == FLT_MAX && aabb_max.x == -FLT_MAX)
        {
            for (const auto& mesh : meshes)
            {
                for (const auto& vertex : mesh.vertices)
                {
                    min = glm::min(min, vertex.Position);
                    max = glm::max(max, vertex.Position);
                }
            }
        }
        else
        {
            min = aabb_min;
            max = aabb_max;
        }
    }
    
private:
    const aiScene* scene;  // 保存场景指针以访问嵌入式纹理
    glm::vec3 aabb_min, aabb_max; // 模型的轴对齐包围盒
    
    // 加载嵌入式纹理
    unsigned int LoadEmbeddedTexture(const aiTexture* aiTex)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        if (aiTex->mHeight == 0)
        {
            // 压缩格式（如 PNG, JPEG）
            int width, height, nrComponents;
            unsigned char* data = stbi_load_from_memory(
                reinterpret_cast<unsigned char*>(aiTex->pcData),
                aiTex->mWidth,
                &width, &height, &nrComponents, 0
            );

            if (data)
            {
                GLenum format;
                if (nrComponents == 1)
                    format = GL_RED;
                else if (nrComponents == 3)
                    format = GL_RGB;
                else if (nrComponents == 4)
                    format = GL_RGBA;

                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);

                stbi_image_free(data);
                
                std::cout << "Loaded embedded texture (compressed): " 
                          << width << "x" << height << " (" << nrComponents << " channels)" << std::endl;
            }
            else
            {
                std::cout << "Failed to load embedded texture (compressed)" << std::endl;
            }
        }
        else
        {
            // 未压缩格式（原始像素数据）
            GLenum format = GL_RGBA;
            glTexImage2D(GL_TEXTURE_2D, 0, format, aiTex->mWidth, aiTex->mHeight, 0, format, GL_UNSIGNED_BYTE, aiTex->pcData);
            glGenerateMipmap(GL_TEXTURE_2D);
            
            std::cout << "Loaded embedded texture (raw): " 
                      << aiTex->mWidth << "x" << aiTex->mHeight << std::endl;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return textureID;
    }
    
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        Assimp::Importer importer;
        scene = importer.ReadFile(path, 
            aiProcess_Triangulate | 
            aiProcess_GenSmoothNormals | 
            aiProcess_FlipUVs | 
            aiProcess_CalcTangentSpace);
        
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        
        directory = path.substr(0, path.find_last_of('/'));

        // 输出加载信息
        std::cout << "========================================" << std::endl;
        std::cout << "Model loaded: " << path << std::endl;
        std::cout << "Directory: " << directory << std::endl;
        std::cout << "Meshes: " << scene->mNumMeshes << std::endl;
        std::cout << "Materials: " << scene->mNumMaterials << std::endl;
        std::cout << "Embedded textures: " << scene->mNumTextures << std::endl;
        std::cout << "========================================" << std::endl;

        processNode(scene->mRootNode, scene);
        
        std::cout << "Model processing complete!" << std::endl;
    }

    void processNode(aiNode *node, const aiScene *scene)
    {
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    // Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    // {
    //     vector<Vertex> vertices;
    //     vector<unsigned int> indices;
    //     vector<Texture> textures;

    //     // 处理顶点
    //     for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    //     {
    //         Vertex vertex;
    //         glm::vec3 vector;
            
    //         // 位置
    //         vector.x = mesh->mVertices[i].x;
    //         vector.y = mesh->mVertices[i].y;
    //         vector.z = mesh->mVertices[i].z;
    //         vertex.Position = vector;
            
    //         // 法线
    //         if (mesh->HasNormals())
    //         {
    //             vector.x = mesh->mNormals[i].x;
    //             vector.y = mesh->mNormals[i].y;
    //             vector.z = mesh->mNormals[i].z;
    //             vertex.Normal = vector;
    //         }
            
    //         // 纹理坐标
    //         if(mesh->mTextureCoords[0])
    //         {
    //             glm::vec2 vec;
    //             vec.x = mesh->mTextureCoords[0][i].x; 
    //             vec.y = mesh->mTextureCoords[0][i].y;
    //             vertex.TexCoords = vec;
                
    //             // 切线
    //             if(mesh->mTangents)
    //             {
    //                 vector.x = mesh->mTangents[i].x;
    //                 vector.y = mesh->mTangents[i].y;
    //                 vector.z = mesh->mTangents[i].z;
    //                 vertex.Tangent = vector;
    //             }
                
    //             // 副切线
    //             if(mesh->mBitangents)
    //             {
    //                 vector.x = mesh->mBitangents[i].x;
    //                 vector.y = mesh->mBitangents[i].y;
    //                 vector.z = mesh->mBitangents[i].z;
    //                 vertex.Bitangent = vector;
    //             }
    //         }
    //         else
    //         {
    //             vertex.TexCoords = glm::vec2(0.0f, 0.0f);
    //         }

    //         vertices.push_back(vertex);
    //     }
        
    //     // 处理索引
    //     for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    //     {
    //         aiFace face = mesh->mFaces[i];
    //         for(unsigned int j = 0; j < face.mNumIndices; j++)
    //             indices.push_back(face.mIndices[j]);        
    //     }
        
    //     // 处理材质
    //     aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
    //     // 1. 漫反射贴图
    //     vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    //     textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
    //     // 2. 镜面反射贴图
    //     vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    //     textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        
    //     // 3. 法线贴图
    //     std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    //     textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        
    //     // 4. 高度贴图
    //     std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    //     textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        
    //     std::cout << " | Mesh: " << (mesh->mName.length > 0 ? mesh->mName.C_Str() : "unnamed") 
    //               << " | Vertices: " << vertices.size() 
    //               << " | Indices: " << indices.size()
    //               << " | Textures: " << textures.size() << std::endl;
        
    //     return Mesh(vertices, indices, textures);
    // }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;
            
            vector.x = mesh->mVertices[i].x;
            vector.y = flipY ? -mesh->mVertices[i].y : mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = flipY ? -mesh->mNormals[i].y : mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            
            if(mesh->mTextureCoords[0])
            {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, 
                                            mesh->mTextureCoords[0][i].y);
                
                if(mesh->mTangents)
                {
                    vector.x = mesh->mTangents[i].x;
                    vector.y = flipY ? -mesh->mTangents[i].y : mesh->mTangents[i].y;
                    vector.z = mesh->mTangents[i].z;
                    vertex.Tangent = vector;
                }
                
                if(mesh->mBitangents)
                {
                    vector.x = mesh->mBitangents[i].x;
                    vector.y = flipY ? -mesh->mBitangents[i].y : mesh->mBitangents[i].y;
                    vector.z = mesh->mBitangents[i].z;
                    vertex.Bitangent = vector;
                }
            }
            else
            {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }
        
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            if (flipY)
            {
                // 反转顶点顺序以保持正确的面朝向
                for(int j = face.mNumIndices - 1; j >= 0; j--)
                    indices.push_back(face.mIndices[j]);
            }
            else
            {
                for(unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }
        }
        
        // 处理材质
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    
        // 1. 漫反射贴图
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
        // 2. 镜面反射贴图
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        
        // 3. 法线贴图
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        
        // 4. 高度贴图
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        
        std::cout << " | Mesh: " << (mesh->mName.length > 0 ? mesh->mName.C_Str() : "unnamed") 
                << " | Vertices: " << vertices.size() 
                << " | Indices: " << indices.size()
                << " | Textures: " << textures.size() << std::endl;
        
        return Mesh(vertices, indices, textures);
    }

    // 支持嵌入式纹理
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            string texturePath = str.C_Str();
            
            // 修正路径（移除绝对路径）
            if(texturePath.find("\\") != string::npos || texturePath.find("/") != string::npos)
            {
                size_t lastSlash = texturePath.find_last_of("\\/");
                texturePath = texturePath.substr(lastSlash + 1);
            }
            
            // 检查是否已加载
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), texturePath.c_str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            
            if(!skip)
            {
                Texture texture;
                
                // 检查是否是嵌入式纹理（Assimp 使用 "*" 前缀）
                if(texturePath[0] == '*')
                {
                    int textureIndex = atoi(texturePath.substr(1).c_str());
                    
                    if(scene && textureIndex >= 0 && textureIndex < static_cast<int>(scene->mNumTextures))
                    {
                        texture.id = LoadEmbeddedTexture(scene->mTextures[textureIndex]);
                        std::cout << "Loaded embedded texture [" << textureIndex << "] for " << typeName << std::endl;
                    }
                    else
                    {
                        std::cout << "ERROR: Embedded texture index out of range: " << textureIndex << std::endl;
                        continue;
                    }
                }
                else
                {
                    // 从文件系统加载纹理
                    texture.id = TextureFromFile(texturePath.c_str(), this->directory);
                    std::cout << "Loaded file texture: " << texturePath << " for " << typeName << std::endl;
                }
                
                texture.type = typeName;
                texture.path = texturePath;
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }
};

// 改进 TextureFromFile 函数
unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
    string filename = string(path);
    
    // 如果不是绝对路径，添加目录前缀
    if(filename.find(':') == string::npos)
    {
        filename = directory + '/' + filename;
    }

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        
        std::cout << "      Texture loaded: " << filename << " (" << width << "x" << height << ")" << std::endl;
    }
    else
    {
        std::cout << "      Texture failed to load at path: " << filename << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

std::unordered_map<std::string, Model*> Model::modelList;

#endif