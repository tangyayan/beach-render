#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <glad/glad.h>
#include <iostream>

class Framebuffer {
public:
    Framebuffer(int width, int height, bool withDepth = true)
        : m_width(width), m_height(height), m_withDepth(withDepth)
    {
        CreateFramebuffer();
    }

    ~Framebuffer() {
        Cleanup();
    }

    void Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glViewport(0, 0, m_width, m_height);
    }

    void Unbind(int screenWidth, int screenHeight) const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screenWidth, screenHeight);
    }

    unsigned int GetTexture() const { return m_texture; }
    unsigned int GetDepthTexture() const { return m_depthTexture; }
    
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    void Resize(int width, int height) {
        if (width == m_width && height == m_height) return;
        
        m_width = width;
        m_height = height;
        Cleanup();
        CreateFramebuffer();
    }

private:
    unsigned int m_fbo = 0;
    unsigned int m_texture = 0;
    unsigned int m_depthTexture = 0;
    unsigned int m_rbo = 0;
    int m_width, m_height;
    bool m_withDepth;

    void CreateFramebuffer() {
        // 创建帧缓冲对象
        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        // 创建颜色纹理附件
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

        if (m_withDepth) {
            // 创建深度纹理附件（用于折射）
            glGenTextures(1, &m_depthTexture);
            glBindTexture(GL_TEXTURE_2D, m_depthTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, m_width, m_height, 0, 
                        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);
        } else {
            // 创建深度渲染缓冲对象（用于反射）
            glGenRenderbuffers(1, &m_rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);
        }

        // 检查帧缓冲完整性
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR: Framebuffer is not complete!" << std::endl;
        } else {
            std::cout << "Framebuffer created successfully: " << m_width << "x" << m_height 
                      << (m_withDepth ? " (with depth texture)" : " (with RBO)") << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Cleanup() {
        if (m_fbo) {
            glDeleteFramebuffers(1, &m_fbo);
            m_fbo = 0;
        }
        if (m_texture) {
            glDeleteTextures(1, &m_texture);
            m_texture = 0;
        }
        if (m_depthTexture) {
            glDeleteTextures(1, &m_depthTexture);
            m_depthTexture = 0;
        }
        if (m_rbo) {
            glDeleteRenderbuffers(1, &m_rbo);
            m_rbo = 0;
        }
    }
};

#endif // FRAMEBUFFER_H