#define _CRT_SECURE_NO_WARNINGS 1
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <myinclude/shader.h>
#include <myinclude/camera.h>
#include <myinclude/FileSystem.h>

#include <skybox.h>

#define screenWidth 800.0f
#define screenHeight 600.0f
int len_id, len_vertex;
bool blinn = false;
bool blinnKeyPressed = false;
float deltaTime, lastFrame;
float lastX = screenWidth / 2.0f;
float lastY = screenHeight / 2.0f;
bool firstMouse = true;
Camera camera(glm::vec3(0.0f, 2.0f, 3.0f));
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
}
// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
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
struct Light {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};
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
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    Light light{
        glm::vec3(1.2f, 1.0f, 2.0f),
        glm::vec3(0.2f, 0.2f, 0.2f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(1.0f, 1.0f, 1.0f)
    };

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

    // Terrain terrain("../../../image/sand_disp.png", 10.0f, 10.0f);
    // terrain.AddTexture(0, "texture_diffuse", "../../../image/sand_diff.jpg");
    // // texture_diffuse1
    // Shader terrainShader("../../../src/terrain.vs", "../../../src/terrain.fs");

    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
           
        /*glm::mat4 lmodel = glm::mat4(1.0f);
        lmodel = glm::rotate(lmodel, (float)glfwGetTime() * glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 lightPos_ = glm::vec3(lmodel * glm::vec4(lightPos, 1.0f));*/
        //用于对光源进行旋转，以实现多次渲染

        skybox.setProjMatrix(glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f));
        skybox.Render();

        // terrainShader.use();
        // glm::mat4 model = glm::mat4(1.0f);
        // glm::mat4 view = camera.GetViewMatrix();
        // glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
        // terrainShader.setMat4("model", model);
        // terrainShader.setMat4("view", view);
        // terrainShader.setMat4("projection", projection);
        // terrainShader.setVec3("viewPos", camera.Position);
        // terrainShader.setVec3("light.position", light.position);
        // terrainShader.setVec3("light.ambient", light.ambient);
        // terrainShader.setVec3("light.diffuse", light.diffuse);
        // terrainShader.setVec3("light.specular", light.specular);
        // terrainShader.setFloat("shininess", 32.0f);
        // terrain.Draw(terrainShader);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}