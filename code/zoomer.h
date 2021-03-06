#pragma once

#include <stdio.h>
#include <string.h>
#include "ch_math.h"
#include "zoomer_input.h"
#include "zoomer_opengl.h"
#include "zoomer_render.h"

struct zoomer
{
    b32 IsMoving;
    v2d StartP;
    v2d P;
    v2d dP;
    f64 Scale;
    f32 IterCount;
    
    rs RS;
    
    f32 Time;
    b32 IsUpdated;
    b32 IsInitialized;
};

global_variable int gWindowWidth;
global_variable int gWindowHeight;