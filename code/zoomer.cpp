/*
TODO(chen):

 1. port to linux
 
*/
#include "zoomer.h"
#include "zoomer_input.cpp"
#include "zoomer_opengl.cpp"
#include "zoomer_render.cpp"

internal void
RunFractalZoomer(zoomer *Zoomer, input PrevInput, input Input, f32 dT)
{
    if (!Zoomer->IsInitialized)
    {
        Zoomer->RS = InitRS();
        
        Zoomer->P = {0.5, 0.0};
        Zoomer->Scale = 0.0;
        Zoomer->IterCount = 200;
        
        Zoomer->IsInitialized = true;
    }
    
    Zoomer->Time += dT;
    HandleInput(Zoomer, PrevInput, Input, dT);
    RenderMandelbrot(Zoomer);
}
