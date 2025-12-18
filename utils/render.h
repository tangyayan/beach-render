#ifndef RENDER_H
#define RENDER_H
#define GLM_SWIZZLE

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <scene.h>
#include <light.h>
#include <camera.h>
#include <vector>
#include <framebuffer.h>
#include <skybox.h>
#include <shadowmap.h>
#include <shader.h>
#include <gameobject.h>

class Render
{
private:
    Scene& main_scene;
    Light& main_light;
    Framebuffer& reflectionFBO;
    Framebuffer& refractionFBO;
    SkyBox& main_skybox;
    ShadowMap shadowMap;

    Shader model_loadingShader;
    Shader shadowShader;
    
    float moveFactor = 0.0f;  // 波纹动画因子

public:
    Render(Scene& main_scene, Light& main_light, Framebuffer& reflectionFBO, 
        Framebuffer& waterfb, SkyBox& main_skybox)
    :   main_scene(main_scene), main_light(main_light), main_skybox(main_skybox),
        reflectionFBO(reflectionFBO), refractionFBO(waterfb),
        shadowMap(4096, 4096),  
        shadowShader(FileSystem::getPath("src/simpleDepthShader.vs").c_str(),
                    FileSystem::getPath("src/simpleDepthShader.fs").c_str()),
        model_loadingShader(FileSystem::getPath("model/model_loading.vs").c_str(),
                            FileSystem::getPath("model/model_loading.fs").c_str())
    {
    }
    
    // 渲染阴影贴图
    void RenderShadowMap(Camera& camera, float screenWidth, float screenHeight)
    {
        glm::mat4 lightSpaceMatrix = main_light.GetLightSpaceMatrix();
        
        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        shadowMap.Bind();
        glCullFace(GL_FRONT);  // Peter Panning 修复

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 10000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        
        // 渲染所有物体到阴影贴图
        auto itr = GameObject::gameObjList.begin();
        for (int i = 0; i < GameObject::gameObjList.size(); i++, ++itr)
        {
            (*itr)->Draw(shadowShader,projection,view);
        }
        
        glCullFace(GL_BACK);
        shadowMap.Unbind(static_cast<int>(screenWidth), static_cast<int>(screenHeight));  // 恢复到屏幕尺寸
    }

    // 渲染反射场景
    void RenderWaterReflection(Camera& camera, float screenWidth, float screenHeight)
    {
        glm::vec3 camPos = camera.Position;
        glm::vec3 camUp = camera.Up;

        float waterHeight = main_scene.GetWaterPlane()->GetHeight();
        glm::vec3 reflectCamPos = glm::vec3(camPos.x, 2 * waterHeight - camPos.y, camPos.z);

        Camera reflectCamera(reflectCamPos, -camUp, camera.Yaw, -camera.Pitch);
        glm::mat4 projMatrix = glm::perspective(glm::radians(reflectCamera.Zoom), screenWidth / screenHeight, 0.1f, 100.0f);

        reflectionFBO.Bind();
        // glEnable(GL_CLIP_DISTANCE0);

        glClearColor(0.5f, 0.7f, 0.9f, 1.0f);  // 天空颜色
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        main_skybox.Render(&reflectCamera, &projMatrix, 1);

        // glDisable(GL_CLIP_DISTANCE0);
        reflectionFBO.Unbind(static_cast<int>(screenWidth), static_cast<int>(screenHeight));
    }
    
    // 渲染折射场景
    void RenderWaterRefraction(Camera& camera, float screenWidth, float screenHeight)
    {
        float waterHeight = main_scene.GetWaterPlane()->GetHeight();
        
        refractionFBO.Bind();
        // glEnable(GL_CLIP_DISTANCE0);

        glClearColor(0.2f, 0.4f, 0.6f, 1.0f);  // 水下颜色
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        main_scene.Draw(
            main_light,
            camera,
            screenWidth,
            screenHeight,
            0.0f,
            glm::vec4(0.0f, -1.0f, 0.0f, waterHeight-50.0f)
        );
        main_skybox.Render(nullptr,nullptr,1);

        // glDisable(GL_CLIP_DISTANCE0);
        refractionFBO.Unbind(static_cast<int>(screenWidth), static_cast<int>(screenHeight));
    }
    
    // 渲染完整场景（地形 + 水面）
    void RenderScene(Camera& camera, float screenWidth, float screenHeight, float time = 0.0f)
    {
        RenderShadowMap(camera, screenWidth, screenHeight);

        glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_CLIP_DISTANCE0);

        main_scene.Draw(
            main_light,
            camera,
            screenWidth,
            screenHeight,
            time,
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            reflectionFBO.GetTexture(),      // 反射纹理
            refractionFBO.GetTexture(),      // 折射纹理
            refractionFBO.GetDepthTexture(), // 深度纹理
            shadowMap.GetDepthMap(),         // 阴影贴图
            main_light.GetLightSpaceMatrix()
        );
        
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 10000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        auto itr = GameObject::gameObjList.begin();
        for (int i = 0; i < GameObject::gameObjList.size(); i++, ++itr)
        {
            (*itr)->Draw(model_loadingShader, projection, view);
        }

        bool isabove = (camera.Position.y > main_scene.GetWaterPlane()->GetHeight());
        main_skybox.Render(nullptr,nullptr,isabove);
    }
    
    // 完整渲染一帧
    void RenderFrame(Camera& camera, float screenWidth, float screenHeight, float time = 0.0f)
    {
        // 1. 渲染反射
        RenderWaterReflection(camera, screenWidth, screenHeight);
        
        // 2. 渲染折射
        RenderWaterRefraction(camera, screenWidth, screenHeight);
        
        // 3. 渲染主场景（包括水面）
        RenderScene(camera, screenWidth, screenHeight, time);

        // main_light.Draw(camera, glm::vec4(0.0f, 1.0f, 0.0f, 1000000.0f));
    }
};

#endif