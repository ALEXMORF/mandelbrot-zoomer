#pragma once

struct rs
{
    GLuint Shader;
    GLuint QuadVAO;
    
    int CurrentFramebufferIndex;
    GLuint Framebuffers[2];
};