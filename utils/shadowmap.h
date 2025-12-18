#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include <glad/glad.h>
#include <iostream>

class ShadowMap
{
private:
    unsigned int depthMapFBO;
    unsigned int depthMap;
    unsigned int shadowWidth;
    unsigned int shadowHeight;

public:
    ShadowMap(unsigned int width = 2048, unsigned int height = 2048)
        : shadowWidth(width), shadowHeight(height)
    {
        // 创建深度贴图 FBO
        glGenFramebuffers(1, &depthMapFBO);
        
        // 创建深度纹理
        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                     shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        // 绑定深度纹理到 FBO
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::SHADOWMAP::Framebuffer is not complete!" << std::endl;
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        std::cout << "ShadowMap created: " << shadowWidth << "x" << shadowHeight << std::endl;
    }

    ~ShadowMap()
    {
        glDeleteFramebuffers(1, &depthMapFBO);
        glDeleteTextures(1, &depthMap);
    }

    void Bind()
    {
        glViewport(0, 0, shadowWidth, shadowHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void Unbind(int screenWidth, int screenHeight)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screenWidth, screenHeight);
    }

    unsigned int GetDepthMap() const { return depthMap; }
    unsigned int GetWidth() const { return shadowWidth; }
    unsigned int GetHeight() const { return shadowHeight; }
};

#endif // SHADOWMAP_H