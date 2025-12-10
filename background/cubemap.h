#ifndef CUBEMAP_H
#define CUBEMAP_H

#include <string>
#include <iostream>
#include <glad/glad.h>
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif
using namespace std;
class CubemapTexture
// 用于加载和绑定立方体贴图
{
public:

    CubemapTexture(const string& Directory,
        const string& PosXFilename,
        const string& NegXFilename,
        const string& PosYFilename,
        const string& NegYFilename,
        const string& PosZFilename,
        const string& NegZFilename)
    {
        m_fileNames[0] = Directory + "/" + PosXFilename;
        m_fileNames[1] = Directory + "/" + NegXFilename;
        m_fileNames[2] = Directory + "/" + PosYFilename;
        m_fileNames[3] = Directory + "/" + NegYFilename;
        m_fileNames[4] = Directory + "/" + PosZFilename;
        m_fileNames[5] = Directory + "/" + NegZFilename;
    }

    ~CubemapTexture()
    {
        glDeleteTextures(1, &m_textureObj);
    }

    bool Load();//上传到gpu，并保存ID

    void Bind(GLenum TextureUnit);//绑定到指定的纹理单元

private:

    string m_fileNames[6];
    GLuint m_textureObj;
};
bool CubemapTexture::Load()
{
    glGenTextures(1, &m_textureObj);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureObj);

    for (unsigned int i = 0 ; i < 6 ; i++) {
        int width, height, nrChannels;
        unsigned char *data = stbi_load(m_fileNames[i].c_str(), &width, &height, &nrChannels, 0);
        if (!data)
        {
            std::cout << "Cubemap texture failed to load at path: " << m_fileNames[i] << std::endl;
            stbi_image_free(data);
            return false;
        }
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format,
            GL_UNSIGNED_BYTE, data);
        
        stbi_image_free(data);//释放图像内存
    } 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);//GL_CLAMP_TO_EDGE 纹理坐标超出范围时，使用边缘颜色

    return true;
}
void CubemapTexture::Bind(GLenum TextureUnit)
{
    glActiveTexture(TextureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureObj);
}
#endif // CUBEMAP_H