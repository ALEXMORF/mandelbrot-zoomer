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
    
    int FrameIndex;
    int CurrentFBOIndex;
    framebuffer Framebuffers[2];
};