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

class Render
{
private:
    Scene& main_scene;
    Light& main_light;
    Framebuffer& reflectionFBO;
    Framebuffer& refractionFBO;
    SkyBox& main_skybox;
    
    float moveFactor = 0.0f;  // 波纹动画因子

public:
    Render(Scene& main_scene, Light& main_light, Framebuffer& reflectionFBO, Framebuffer& waterfb, SkyBox& main_skybox)
        : main_scene(main_scene), main_light(main_light),main_skybox(main_skybox),
          reflectionFBO(reflectionFBO), refractionFBO(waterfb)
    {
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
            refractionFBO.GetDepthTexture() // 深度纹理
        );
        bool isabove = (camera.Position.y > main_scene.GetWaterPlane()->GetHeight());
        main_skybox.Render(nullptr,nullptr,isabove);
    }

    void Drawobject(Camera& camera, Shader& shader, glm::mat4 model, float screenWidth, float screenHeight)
    {
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom), 
            screenWidth / screenHeight, 
            0.1f, 
            10000.0f
        );

        shader.use();
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
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