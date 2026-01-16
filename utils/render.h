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
#include <cube.h>

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

    Cube* sunCube = nullptr;
    float currentDayFactor = 1.0f;

    void UpdateDayNight(float time, const Camera& camera)
    {
        float cycle = 60.0f;
        float t = fmod(time + 1.0f, cycle) / cycle;

        float angle = t * glm::radians(360.0f) - glm::radians(90.0f);
        float radius = 7500.0f;

        // 先在 XZ 平面
        glm::vec3 basePos(
            radius * cos(angle),
            0.0f,
            radius * sin(angle)
        );

        // 绕 X 轴稍微抬一点（控制高度变化方向）
        float tiltX = glm::radians(60.0f);
        // 绕 Y 轴决定“从哪个方位升起”
        float tiltY = glm::radians(30.0f);

        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), tiltX, glm::vec3(1, 0, 0));
        rot = glm::rotate(rot, tiltY, glm::vec3(0, 1, 0));

        glm::vec3 sunPos = glm::vec3(rot * glm::vec4(basePos, 1.0f));

        // 月亮位置：反向，简单用 -sunPos
        glm::vec3 moonPos = -sunPos;

        // 高度 [-1,1]
        float h = glm::clamp(sunPos.y / radius, -1.0f, 1.0f);
        float dayFactor = glm::smoothstep(-0.2f, 0.2f, h);

        // 更新方向光（白天亮、夜晚暗）
        glm::vec3 ambient = glm::mix(glm::vec3(0.02f, 0.02f, 0.05f),
            glm::vec3(0.3f, 0.3f, 0.25f),
            dayFactor);
        glm::vec3 diffuse = glm::mix(glm::vec3(0.05f, 0.05f, 0.1f),
            glm::vec3(0.9f, 0.85f, 0.8f),
            dayFactor);
        glm::vec3 specular = glm::mix(glm::vec3(0.1f),
            glm::vec3(0.9f),
            dayFactor);

        glm::vec3 lightDirWorld;
        if (sunPos.y >= 0.0f)
            lightDirWorld = -glm::normalize(sunPos);   // 白天：太阳方向
        else
            lightDirWorld = -glm::normalize(moonPos); // 夜晚：月亮方向
        main_light.UpdateDirLight(lightDirWorld, ambient, diffuse, specular);

        currentDayFactor = dayFactor;

        if (sunCube != nullptr)
        {
            // 判断当前是“太阳在上方”还是“月亮在上方”
            bool sunAbove = (sunPos.y >= 0.0f);

            // 设置颜色：太阳 = 土黄色，月亮 = 冷白/淡蓝
            glm::vec3 sunColor = glm::vec3(0.8f, 0.7f, 0.3f); // 土黄色
            glm::vec3 moonColor = glm::vec3(0.7f, 0.8f, 1.0f); // 月亮偏冷色

            Shader& lightShader = main_light.GetLightboxShader();

            glm::mat4 model(1.0f);
            glm::vec4 noClip(0.0f);

            if (sunAbove) // 画太阳
            {
                glm::mat4 modelSun(1.0f);
                modelSun = glm::translate(modelSun, sunPos);
                modelSun = glm::scale(modelSun, glm::vec3(200.0f)); // 太阳大小

                lightShader.use();
                lightShader.setVec3("lightColor", sunColor);
                sunCube->Draw(const_cast<Camera&>(camera), noClip, modelSun, false);
            }
            else // 画月亮（与太阳相对）
            {
                glm::mat4 modelMoon(1.0f);
                modelMoon = glm::translate(modelMoon, moonPos);
                modelMoon = glm::scale(modelMoon, glm::vec3(150.0f)); // 略小一点

                lightShader.use();
                lightShader.setVec3("lightColor", moonColor);
                sunCube->Draw(const_cast<Camera&>(camera), noClip, modelMoon, false);
            }
        }

        // Debug 输出
        std::cout << "sunPos: " << sunPos.x << " " << sunPos.y << " " << sunPos.z << std::endl;
    }

public:
    Render(Scene& main_scene, Light& main_light, Framebuffer& reflectionFBO,
        Framebuffer& waterfb, SkyBox& main_skybox)
        : main_scene(main_scene), main_light(main_light), main_skybox(main_skybox),
        reflectionFBO(reflectionFBO), refractionFBO(waterfb),
        shadowMap(4096, 4096),
        shadowShader(FileSystem::getPath("src/simpleDepthShader.vs").c_str(),
            FileSystem::getPath("src/simpleDepthShader.fs").c_str()),
        model_loadingShader(FileSystem::getPath("model/model_loading.vs").c_str(),
            FileSystem::getPath("model/model_loading.fs").c_str())
    {
        std::vector<Texture> dummyTextures;
        sunCube = new Cube(dummyTextures, main_light.GetLightboxShader());
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
            if((*itr)->isSelected && GameObject::movingObject)continue;
            (*itr)->snaptoterrain(main_scene.GetTerrain());
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

        main_skybox.Render(&reflectCamera, &projMatrix, 1, currentDayFactor);

        // glDisable(GL_CLIP_DISTANCE0);
        reflectionFBO.Unbind(static_cast<int>(screenWidth), static_cast<int>(screenHeight));
    }
    
    // 渲染折射场景
    void RenderWaterRefraction(Camera& camera, float screenWidth, float screenHeight)
    {
        float waterHeight = main_scene.GetWaterPlane()->GetHeight();
        
        refractionFBO.Bind();
        glEnable(GL_CLIP_DISTANCE0);

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
        // 渲染所有物体到折射纹理
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 10000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        auto itr = GameObject::gameObjList.begin();
        for (int i = 0; i < GameObject::gameObjList.size(); i++, ++itr)
        {
            if((*itr)->isSelected && GameObject::movingObject)continue;
            if(camera.Position.y > waterHeight)
            {
                (*itr)->Draw(model_loadingShader,projection,view, glm::vec4(0, -1, 0, waterHeight));
            }
            else
            {
                (*itr)->Draw(model_loadingShader,projection,view,glm::vec4(0, 1, 0, -waterHeight));
            }
        }
        main_skybox.Render(nullptr, nullptr, 1, currentDayFactor);

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

        // 新增：更新昼夜 & 画太阳立方体
        UpdateDayNight(time, camera);

        // main_scene.Draw 内部会调用 light.SetLight
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
        main_light.SetLight(model_loadingShader);
        model_loadingShader.setVec3("viewPos", camera.Position);
        for (int i = 0; i < GameObject::gameObjList.size(); i++, ++itr)
        {
            if((*itr)->isSelected && GameObject::movingObject)continue;
            (*itr)->Draw(model_loadingShader, projection, view);
        }

        bool isabove = (camera.Position.y > main_scene.GetWaterPlane()->GetHeight());
        main_skybox.Render(nullptr, nullptr, isabove, currentDayFactor);
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

        //main_light.Draw(camera, glm::vec4(0.0f, 1.0f, 0.0f, 1000000.0f));
    }
};

#endif