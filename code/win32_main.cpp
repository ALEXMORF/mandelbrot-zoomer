#include <windows.h>
#include "ch_gl.h"

#include "kernel.h"
#include "zoomer.cpp"

#include "win32_kernel.h"

global_variable b32 gAppIsRunning;

LRESULT CALLBACK
Win32WindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch (Message)
    {
        case WM_CLOSE:
        case WM_QUIT:
        {
            gAppIsRunning = false;
        } break;
        
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

int CALLBACK 
WinMain(HINSTANCE CurrentInstance,
        HINSTANCE PreviousInstance,
        LPSTR Commandline,
        int ShowCode)
{
    HWND Window = Win32CreateWindow(CurrentInstance, 1080, 724,
                                    "Fractal-Zoom", "Fractal-Zoom windowclass",
                                    Win32WindowCallback);
    HDC WindowDC = GetDC(Window);
    Win32InitializeOpengl(WindowDC, 4, 0);
    wglSwapInterval(1);
    LoadGLFunctions(Win32GetOpenglFunction);
    
    input Input = {};
    zoomer Zoomer = {};
    
    gAppIsRunning = true;
    while (gAppIsRunning)
    {
        MSG Message = {};
        while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
        {
            switch (Message.message)
            {
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                {
                    WPARAM KeyCode = Message.wParam;
                    b32 KeyIsDown = (Message.lParam & (1 << 31)) == 0;
                    b32 KeyWasDown = (Message.lParam & (1 << 30)) != 0;
                    b32 AltIsDown = (Message.lParam & (1 << 29)) != 0;
                    
                    if (!KeyWasDown && KeyIsDown)
                    {
                        if (AltIsDown && KeyCode == VK_F4)
                        {
                            gAppIsRunning = false;
                        }
                    }
                } break;
                
                default:
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                } break;
            }
        }
        
        RunFractalZoomer(&Zoomer, Input);
        
        SwapBuffers(WindowDC);
        Sleep(5);
    }
    
    return 0;
}