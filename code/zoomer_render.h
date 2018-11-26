#pragma once

struct framebuffer
{
    GLuint Handle;
    GLuint TexHandle;
    GLuint RBOHandle;
};

struct rs
{
    GLuint MandelbrotShader;
    GLuint BlitShader;
    GLuint QuadVAO;
    
    int CurrentFramebufferIndex;
    framebuffer Framebuffers[2];
};