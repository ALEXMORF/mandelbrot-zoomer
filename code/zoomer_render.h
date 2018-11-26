#pragma once

struct rs
{
    GLuint MandelbrotShader;
    GLuint QuadVAO;
    
    int CurrentFramebufferIndex;
    GLuint Framebuffers[2];
};