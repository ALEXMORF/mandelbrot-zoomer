#pragma once
#include "ch_math.h"

struct input
{
    b32 ShiftIsDown;
    b32 SpaceIsDown;
    
    b32 MouseIsDown;
    v2 MouseP;
};

struct zoomer
{
    //logic
    b32 IsMoving;
    v2d StartP;
    v2d P;
    f64 Scale;
    
    //render
    GLuint QuadVAO;
    GLuint Shader;
    
    b32 IsInitialized;
};