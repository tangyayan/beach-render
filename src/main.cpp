#define _CRT_SECURE_NO_WARNINGS 1
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <light.h>

#include <myinclude/shader.h>
#include <myinclude/camera.h>
#include <myinclude/FileSystem.h>

#include <skybox.h>
#include <render.h>
#include <framebuffer.h>

#include <gameobject.h>

#define screenWidth 800.0f
#define screenHeight 600.0f
int len_id, len_vertex;
bool blinn = false;
bool blinnKeyPressed = false;
float deltaTime, lastFrame;
float lastX = screenWidth / 2.0f;
float lastY = screenHeight / 2.0f;
bool firstMouse = true;

float worldTime = 0.0f;      // 昼夜系统用的“世界时间”
float timeScale = 0.2f;      // 默认时间倍率（<1 表示比真实时间慢）
bool fastTime = false;       // 是否处于加速状态

Camera camera(glm::vec3(0.0f, 30.0f, 50.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -45.0f);
void processInput(GLFWwindow* window)
{
    /*
    * 按B切换光照模型
    * wsad键用于控制摄像机的前进方向
    */
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !blinnKeyPressed)
    {
        blinn = !blinn;
        blinnKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        blinnKeyPressed = false;
    }

    // 按 F 键切换时间快/慢
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fastTime)
    {
        fastTime = true;
        timeScale = 5.0f;   // 快进倍率
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE && fastTime)
    {
        fastTime = false;
        timeScale = 0.2f;   // 恢复到慢速
    }
}

// 屏幕坐标转世界射线
void ScreenToWorldRay(double mouseX, double mouseY, Camera& camera,
                      glm::vec3& rayOrigin, glm::vec3& rayDir)
{
    // 归一化设备坐标
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;
    
    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);//近平面
    
    // 视图空间
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera.Zoom), 
                                                  (float)screenWidth / screenHeight, 
                                                  0.1f, 100.0f);
    glm::vec4 rayEye = glm::inverse(projectionMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    
    // 世界空间
    glm::mat4 viewMatrix = camera.GetViewMatrix();
    glm::vec3 rayWorld = glm::vec3(glm::inverse(viewMatrix) * rayEye);
    rayDir = glm::normalize(rayWorld);
    rayOrigin = camera.Position;
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
GameObject* GameObject::movingObject = nullptr;
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if(GameObject::selectedObject != nullptr && GameObject::selectedObject->isGround)
    {
        glm::vec3 rayOrigin, rayDir;
        ScreenToWorldRay(xposIn, yposIn, camera, rayOrigin, rayDir);
        float terrainHeight = 0.0f;//因为地面平缓，所以偷懒了
        float t = (terrainHeight - rayOrigin.y) / rayDir.y;
        float intersectX = rayOrigin.x + rayDir.x * t;
        float intersectZ = rayOrigin.z + rayDir.z * t;
        glm::vec3 newPos = glm::vec3(intersectX, terrainHeight, intersectZ);
        if(GameObject::movingObject == nullptr)
        {
            GameObject::movingObject = new GameObject(
                GameObject::selectedObject->modelPath,
                GameObject::selectedObject->modelMat,
                newPos,
                GameObject::selectedObject->rotY,
                GameObject::selectedObject->sca
            );
            GameObject::movingObject->isSelected = false;
            GameObject::movingObject->Update();
            std::cout<<GameObject::gameObjList.size()<<std::endl;
        }
        else{
            GameObject::movingObject->pos = newPos;
            GameObject::movingObject->Update();
        }
    }

    /*
    * 鼠标在按住时拖动可以改变摄像机的欧拉角（俯仰角和偏航角）
    */
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
    {
        firstMouse = true;
        return;
    }

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: mouse button
// 添加一个鼠标按钮回调函数
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        // 屏幕坐标转换为世界坐标射线
        glm::vec3 rayOrigin, rayDir;
        ScreenToWorldRay(xpos, ypos, camera, rayOrigin, rayDir);
        std::cout << "rayOrigin = ("
        << rayOrigin.x << ", "
        << rayOrigin.y << ", "
        << rayOrigin.z << ")\n";

        std::cout << "rayDir = ("
                << rayDir.x << ", "
                << rayDir.y << ", "
                << rayDir.z << ")\n";

        if(GameObject::selectedObject != nullptr && GameObject::selectedObject->isGround)
        {
            float terrainHeight = 0.0f;//因为地面平缓，所以偷懒了
            float t = (terrainHeight - rayOrigin.y) / rayDir.y;
            float intersectX = rayOrigin.x + rayDir.x * t;
            float intersectZ = rayOrigin.z + rayDir.z * t;
            GameObject::selectedObject->pos = glm::vec3(intersectX, terrainHeight, intersectZ);
            GameObject::selectedObject->Update();
            
            GameObject::selectedObject->Deselect();
            if(GameObject::movingObject != nullptr)
            {
                delete GameObject::movingObject;
                GameObject::movingObject = nullptr;
            }
            return;
        }

        // 检测所有物体
        GameObject* closestObject = nullptr;
        float closestDistance = FLT_MAX;
        
        for (auto* obj : GameObject::gameObjList)
        {
            if (obj->RayIntersect(rayOrigin, rayDir))
            {
                float distance = glm::length(obj->pos - rayOrigin);
                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    closestObject = obj;
                }
            }
        }
        
        if (closestObject != nullptr)
        {
            printf("select\n");
            closestObject->Select();
        }
        else if (GameObject::selectedObject != nullptr)
            GameObject::selectedObject->Deselect();
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    /*
    * 可以缩放视角
    */
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Hello OpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_FRAMEBUFFER_SRGB);

    // Light light{
    //     // glm::vec3(1.2f, 50.0f, 100.0f),
    //     glm::vec3(0.0f, -1.0f, -1.0f),
    //     glm::vec3(1.0f, 1.0f, 1.0f),
    //     glm::vec3(0.8f, 0.8f, 0.8f),
    //     glm::vec3(1.0f, 1.0f, 1.0f)
    // };
    Light light(
        DirLight{
            glm::vec3(0.0f, -0.9f, -0.4f),
            glm::vec3(0.2f, 0.2f, 0.2f),
            glm::vec3(0.8f, 0.8f, 0.8f),
            glm::vec3(0.5f, 0.5f, 0.5f)
        },
        *(new Shader(FileSystem::getPath("background/light.vs").c_str(),
                     FileSystem::getPath("background/light.fs").c_str()))
    );
    light.AddPointLight(PointLight{
        glm::vec3(-4.0f,  2.5f, -8.0f),
        1.0f, 0.09f, 0.032f,
        glm::vec3(0.2f, 0.2f, 0.2f),
        glm::vec3(0.8f, 0.8f, 0.8f),
        glm::vec3(1.0f, 1.0f, 1.0f)
        }
    );
    light.AddPointLight(PointLight{
        glm::vec3(2.0f, 40.0f, 10.0f),
        1.0f, 0.09f, 0.032f,
        glm::vec3(0.2f, 0.2f, 0.2f),
        glm::vec3(0.8f, 0.8f, 0.8f),
        glm::vec3(1.0f, 1.0f, 1.0f)
        }
    );
    light.AddPointLight(PointLight{
        glm::vec3(2.3f, 2.0f, -5.0f),
        1.0f, 0.09f, 0.032f,
        glm::vec3(0.2f, 0.2f, 0.2f),
        glm::vec3(0.8f, 0.8f, 0.8f),
        glm::vec3(1.0f, 1.0f, 1.0f)
        }
    );
    light.AddPointLight(PointLight{
        glm::vec3(0.0f,  1.5f, -3.0f),
        1.0f, 0.09f, 0.032f,
        glm::vec3(0.2f, 0.2f, 0.2f),
        glm::vec3(0.8f, 0.8f, 0.8f),
        glm::vec3(1.0f, 1.0f, 1.0f)
        }
    );

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);//线框模式
    bool last_blinn = false;
    //int framcnt = 0;
    //float startTime = static_cast<float>(glfwGetTime());
    SkyBox skybox(&camera, glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f));
    bool sky_init = skybox.Init(FileSystem::getPath("image/skybox"),
        "xp.jpg",
        "xn.jpg",
        "yp.jpg",
        "yn.jpg",
        "zp.jpg",
        "zn.jpg");
    if(!sky_init)
    {
        std::cout << "Failed to initialize skybox" << std::endl;
        return -1;
    }

    Scene scene({
        FileSystem::getPath("image/sand_disp.png"),
        FileSystem::getPath("image/sand_diff.jpg")
        }, 6.0f, 1.0f, 1.0f);
    
    Render renderer(scene, light,
        *(new Framebuffer(screenWidth, screenHeight, false)),
        *(new Framebuffer(screenWidth, screenHeight, true)),
        skybox
    );

    // glm::mat4 model = glm::mat4(1.0f);
    glm::vec3 coco_pos = glm::vec3(0.0f, 0.0f, -0.3f);
    glm::vec3 coco_scale = glm::vec3(0.05f, 0.05f, 0.05f);
    // model = glm::translate(model, glm::vec3(0.0f, 0.0f, -0.3f)); // translate it down so it's at the center of the scene
    // model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));	// it's a bit too big for our scene, so scale it down
    GameObject coconut(FileSystem::getPath("model/coconut_palm.glb"), 
        glm::mat4(1.0f), coco_pos, 0.0f, coco_scale);
    coconut.Update();
    
    glm::vec3 orca_pos = glm::vec3(2.0f, -5.0f, 100.0f);
    glm::vec3 orca_scale = glm::vec3(0.01f, 0.01f, 0.01f);
    GameObject orca(FileSystem::getPath("model/orca.glb"),
        glm::mat4(1.0f),orca_pos, 0.0f, orca_scale, false, true);
    orca.Update();

    glm::vec3 chair_pos = glm::vec3(-5.0f, 0.0f, -30.0f);
    glm::vec3 chair_scale = glm::vec3(0.1f, 0.1f, 0.1f);
    GameObject chair(FileSystem::getPath("model/cover-chair/Cover_Chair.obj"),
        glm::mat4(1.0f), chair_pos, 0.0f, chair_scale);
    chair.Update();

    glm::vec3 umbrella_pos = glm::vec3(20.0f, 0.0f, -30.0f);
    glm::vec3 umbrella_scale = glm::vec3(1.5f, 1.5f, 1.5f);
    GameObject umbrella(FileSystem::getPath("model/beach_table.glb"),
        glm::mat4(1.0f), umbrella_pos, 0.0f, umbrella_scale);
    umbrella.Update();

    // Shader modelShader(FileSystem::getPath("model/model_loading.vs").c_str(),
                    //   FileSystem::getPath("model/model_loading.fs").c_str());

    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        worldTime += deltaTime * timeScale;

        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
           
        /*glm::mat4 lmodel = glm::mat4(1.0f);
        lmodel = glm::rotate(lmodel, (float)glfwGetTime() * glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 lightPos_ = glm::vec3(lmodel * glm::vec4(lightPos, 1.0f));*/
        //用于对光源进行旋转，以实现多次渲染
        // scene.Draw(light, camera, screenWidth, screenHeight, currentFrame);
        renderer.RenderFrame(camera, static_cast<float>(screenWidth), static_cast<float>(screenHeight), worldTime);

        // glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 10000.0f);
        // glm::mat4 view = camera.GetViewMatrix();
        // coconut.Draw(modelShader, projection, view);

        // skybox.setProjMatrix(glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f));
        // skybox.Render();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}