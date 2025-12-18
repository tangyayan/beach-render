#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <shader.h>
#include <texture.h>
#include <FileSystem.h>
#include "terrain.h"
#include "waterplane.h"
#include <ocean_fft_baker.h>
#include <waterplane_baked.h>
#include "light.h"
#include "camera.h"

using namespace std;

class Scene
{
private:
    Terrain* terrain;
    // WaterPlane* waterPlane;  // 水面对象
    OceanBaked* waterPlane;
    OceanFFTBaker* baker;
    Shader terrainShader;
    Shader waterShader; 
        
    float sandheight;
    float groundscale;
    float waterLevel; 

public:
    Scene(vector<string> ground_path, 
          float sandheight = 10.0f, 
          float groundscale = 1.0f,
          float waterLevel = 0.0f)
        : sandheight(sandheight), 
          groundscale(groundscale),
          waterLevel(waterLevel),
          terrainShader(
              FileSystem::getPath("background/terrain.vs").c_str(),
              FileSystem::getPath("background/terrain.fs").c_str()
          ),
          waterShader(  // 初始化水面着色器
              FileSystem::getPath("background/water.vs").c_str(),
              FileSystem::getPath("background/water.fs").c_str()
          )
    {
        InitializeScene(ground_path);
    }

    ~Scene()
    {
        delete terrain;
        if (waterPlane) {
            delete waterPlane;
        }
        if (baker) {
            delete baker;
        }
    }

    void Draw(Light &light, Camera &camera, float screenWidth, float screenHeight, float time = 0.0f,
              glm::vec4 clipping_plane = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
              unsigned int reflectionTexture = 0,
              unsigned int refractionTexture = 0,
              unsigned int depthTexture = 0,
              unsigned int shadowMap = 0,
              glm::mat4 lightSpaceMatrix = glm::mat4(1.0f)
            ) 
    {
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom), 
            screenWidth / screenHeight, 
            0.1f, 
            10000.0f
        );

        int is_above = (camera.Position.y > waterPlane->GetHeight());

        terrainShader.use();
        glm::mat4 model = glm::mat4(1.0f);
        terrainShader.setMat4("model", model);
        terrainShader.setMat4("view", view);
        terrainShader.setMat4("projection", projection);
        terrainShader.setVec3("viewPos", camera.Position);
        // terrainShader.setVec3("light.direction", light.direction);
        // terrainShader.setVec3("light.ambient", light.ambient);
        // terrainShader.setVec3("light.diffuse", light.diffuse);
        // terrainShader.setVec3("light.specular", light.specular);
        light.SetLight(terrainShader);
        terrainShader.setFloat("shininess", 32.0f);
        terrainShader.setInt("isAbove", is_above);

        // shadow
        terrainShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        terrainShader.setInt("shadowMap", 3);
        
        terrain->Draw(terrainShader);

        if (waterPlane && clipping_plane == glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)) {
            // 启用混合以支持半透明
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            waterShader.use();
            waterShader.setMat4("model", model);
            waterShader.setMat4("view", view);
            waterShader.setMat4("projection", projection);
            waterShader.setVec3("viewPos", camera.Position);
            // waterShader.setVec3("light.direction", light.direction);
            // waterShader.setVec3("light.ambient", light.ambient);
            // waterShader.setVec3("light.diffuse", light.diffuse);
            // waterShader.setVec3("light.specular", light.specular);
            light.SetLight(waterShader);
            // waterShader.setFloat("time", time);
            // waterShader.setVec3("waterColor", glm::vec3(0.0f, 0.5f, 0.7f));  // 蓝绿色
            // waterShader.setVec3("waterColor", glm::vec3(0.5f, 0.6f, 0.8f));  // 淡蓝色
            // waterShader.setVec3("waterColor_diffuse", glm::vec3(0.8f, 0.9f, 1.0f));
            waterShader.setFloat("shininess", 32.0f);
            waterShader.setInt("isAbove", is_above);
            // printf("isAbove: %d\n", is_above);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, reflectionTexture);
            waterShader.setInt("reflectionTexture", 0);
            
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, refractionTexture);
            waterShader.setInt("refractionTexture", 1);
            
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, depthTexture);
            waterShader.setInt("depthTexture", 2);
            
            waterPlane->Draw(waterShader, time);
            
            glDisable(GL_BLEND);
        }
    }

    float GetHeight(float x, float z) const
    {
        return terrain->GetHeight(x, z);
    }

    Terrain* GetTerrain() const { return terrain; }
    // WaterPlane* GetWaterPlane() const { return waterPlane; }
    OceanBaked* GetWaterPlane() const { return waterPlane; }

private:
    void InitializeScene(vector<string> ground_path)
    {
        std::cout << "\n=== Initializing Scene ===" << std::endl;
        
        string heightmapPath = ground_path[0];
        vector<string> texture_paths(ground_path.begin() + 1, ground_path.end());
        
        TextureManager textureManager(GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
        
        vector<string> types;
        for(size_t i = 0; i < texture_paths.size(); i++)
        {
            types.push_back("texture_diffuse");
        }
        
        vector<Texture> textures = textureManager.LoadTexture(texture_paths, types);
        
        std::cout << "Creating terrain..." << std::endl;
        terrain = new Terrain(heightmapPath, textures, sandheight, groundscale, 8, -20.0f, 100.0f);
        
        // 创建水面
        std::cout << "Creating water plane..." << std::endl;
        
        // 获取地形尺寸
        float terrainWidth = terrain->GetWidth() * groundscale;
        float terrainLength = terrain->GetHeight() * groundscale / 2.0f;  // Z轴正半轴
        
        // 创建水面 (可选: 使用纹理或纯色)
        vector<Texture> waterTextures;  // 空纹理列表,使用纯色
        
        // waterPlane = new WaterPlane(
        //     terrainWidth,      // 宽度 (覆盖整个地形宽度)
        //     terrainLength,     // 长度 (Z轴正方向的长度)
        //     waterLevel,        // 高度 (海平面高度)
        //     100,               // 网格细分 (越大波浪越平滑)
        //     waterTextures      // 水面纹理
        // );
        baker = new OceanFFTBaker(
            256,             // 空间分辨率
            32,              // 时间帧数 (32 帧)
            5.0f,            // 时间跨度 (5 秒循环)
            256,           // L
            0.5f,         // Phillips 谱振幅
            glm::vec2(0.0f, -1.0f),  // 风向
            30.0f            // 风速
        );
        waterPlane = new OceanBaked(
            128,
            terrainWidth/2,
            terrainLength,
            waterLevel,
            baker->GetDisplacementTexture(),
            baker->GetNormalTexture(),
            baker->GetTimeSpan()
        );
        
        std::cout << "Scene initialization complete!" << std::endl;
    }
};

#endif // SCENE_H