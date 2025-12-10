#ifndef SKYBOX_H_
#define SKYBOX_H_

#include <camera.h>
#include <shader.h>
#include <cubemap.h>
#include <FileSystem.h>

class SkyBox
{
public:
    SkyBox(const Camera* pCamera, glm::mat4 p)
    {
        m_pCamera = pCamera;
        m_ProjMatrix = p;
        m_pSkyboxShader = new Shader(FileSystem::getPath("background/skybox.vs").c_str(), 
                                    FileSystem::getPath("background/skybox.fs").c_str());
        m_skyboxVAO = 0;
        m_skyboxVBO = 0;
        SetupSkyboxMesh();
    }

    ~SkyBox();

    bool Init(const string& Directory,
        const string& PosXFilename,
        const string& NegXFilename,
        const string& PosYFilename,
        const string& NegYFilename,
        const string& PosZFilename,
        const string& NegZFilename);

    void setProjMatrix(const glm::mat4& p)
    {
        m_ProjMatrix = p;
    }

    void Render();

private:
    void SetupSkyboxMesh();
    
    Shader* m_pSkyboxShader;
    const Camera* m_pCamera;
    CubemapTexture* m_pCubemapTex;
    glm::mat4 m_ProjMatrix;
    unsigned int m_skyboxVAO;
    unsigned int m_skyboxVBO;
};

void SkyBox::SetupSkyboxMesh()
{
    // 天空盒立方体顶点数据
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // 创建 VAO 和 VBO
    glGenVertexArrays(1, &m_skyboxVAO);
    glGenBuffers(1, &m_skyboxVBO);
    
    glBindVertexArray(m_skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    
    // 位置属性
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

SkyBox::~SkyBox()
{
    delete m_pSkyboxShader;
    delete m_pCubemapTex;
    
    // 清理 OpenGL 资源
    if (m_skyboxVAO != 0) {
        glDeleteVertexArrays(1, &m_skyboxVAO);
    }
    if (m_skyboxVBO != 0) {
        glDeleteBuffers(1, &m_skyboxVBO);
    }
}

bool SkyBox::Init(const string& Directory,
    const string& PosXFilename,
    const string& NegXFilename,
    const string& PosYFilename,
    const string& NegYFilename,
    const string& PosZFilename,
    const string& NegZFilename)
{
    m_pCubemapTex = new CubemapTexture(Directory,
        PosXFilename,
        NegXFilename,
        PosYFilename,
        NegYFilename,
        PosZFilename,
        NegZFilename);
    return m_pCubemapTex->Load();
}

void SkyBox::Render()
{
    // 保存旧的状态
    GLint OldCullFaceMode;
    glGetIntegerv(GL_CULL_FACE_MODE, &OldCullFaceMode);
    GLint OldDepthFuncMode;
    glGetIntegerv(GL_DEPTH_FUNC, &OldDepthFuncMode);

    glCullFace(GL_FRONT); // 天空盒是内向的,所以要剔除前面
    glDepthFunc(GL_LEQUAL); // 天空盒深度值是1.0,设置深度函数为小于等于

    m_pSkyboxShader->use();
    
    // 设置变换矩阵
    glm::mat4 model = glm::mat4(1.0f);
    // model = glm::translate(model, m_pCamera->Position); // 将天空盒移动到相机位置
    // model = glm::scale(model, glm::vec3(20.0f)); // 放大天空盒
    glm::mat4 view = glm::mat4(glm::mat3(m_pCamera->GetViewMatrix()));
    
    m_pSkyboxShader->setMat4("model", model);
    m_pSkyboxShader->setMat4("view", view);
    m_pSkyboxShader->setMat4("projection", m_ProjMatrix);
    m_pSkyboxShader->setInt("skybox", 0); // 纹理单元0
    
    // 绑定立方体贴图
    m_pCubemapTex->Bind(GL_TEXTURE0);
    
    // 渲染天空盒立方体
    glBindVertexArray(m_skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36); // 36个顶点 (6面 * 2三角形 * 3顶点)
    glBindVertexArray(0);

    // 恢复旧的状态
    glCullFace(OldCullFaceMode);
    glDepthFunc(OldDepthFuncMode);
}

#endif // SKYBOX_H_