#include "ch_gl.h"
#undef APIENTRY
#include "kernel.h"
#include "zoomer.cpp"

#include <windows.h>
#include <windowsx.h>
#include <ShellScalingAPI.h>
#include "win32_kernel.h"

//TODO(chen): double-buffered input state
global_variable input gPrevInput;
global_variable input gInput;
global_variable b32 gAppIsRunning;

LRESULT CALLBACK
Win32WindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch (Message)
    {
        case WM_SIZE:
        {
            gWindowWidth = LParam & 0xffff;
            gWindowHeight = (int)(LParam >> 16);
        } break;
        
        case WM_LBUTTONDOWN:
        {
            gInput.MouseIsDown = true;
        } break;
        
        case WM_LBUTTONUP:
        {
            gInput.MouseIsDown = false;
        } break;
        
        case WM_MOUSEMOVE:
        {
            int MouseX = GET_X_LPARAM(LParam); 
            int MouseY = GET_Y_LPARAM(LParam);
            RECT ClientRect = Win32GetClientRect(Window);
            
            gInput.MouseP.X = (f32)(MouseX - ClientRect.left) / (f32)(ClientRect.right - ClientRect.left);
            gInput.MouseP.Y = (f32)(MouseY - ClientRect.top) / (f32)(ClientRect.bottom - ClientRect.top);
        } break;
        
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
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
    gWindowWidth = 1080;
    gWindowHeight = 724;
    HWND Window = Win32CreateWindow(CurrentInstance, 
                                    gWindowWidth, gWindowHeight,
                                    "Fractal-Zoom", "Fractal-Zoom windowclass",
                                    Win32WindowCallback);
    ASSERT(Window);
    HDC WindowDC = GetDC(Window);
    b32 InitializedOpenGL = Win32InitializeOpengl(WindowDC, 4, 0);
    ASSERT(InitializedOpenGL);
    wglSwapInterval(1);
    LoadGLFunctions(Win32GetOpenglFunction);
    
    zoomer Zoomer = {};
    u64 LastTimeCounter = Win32GetPerformanceCounter();
    
    gAppIsRunning = true;
    while (gAppIsRunning)
    {
        gPrevInput = gInput;
        
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
                    
                    if (KeyWasDown != KeyIsDown)
                    {
                        if (AltIsDown && KeyCode == VK_F4)
                        {
                            gAppIsRunning = false;
                        }
                        
                        if (KeyIsDown && AltIsDown && KeyCode == VK_RETURN)
                        {
                            Win32ToggleFullscreen(Window);
                        }
                        
                        if (KeyCode == VK_SHIFT)
                        {
                            gInput.ShiftIsDown = KeyIsDown;
                        }
                        
                        if (KeyCode == VK_UP)
                        {
                            gInput.UpArrowIsDown = KeyIsDown;
                        }
                        
                        if (KeyCode == VK_DOWN)
                        {
                            gInput.DownArrowIsDown = KeyIsDown;
                        }
                        
                        if (KeyCode == VK_SPACE)
                        {
                            gInput.SpaceIsDown = KeyIsDown;
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
        
        f32 dT = Win32GetTimeElapsedInMS(LastTimeCounter, Win32GetPerformanceCounter());
        LastTimeCounter = Win32GetPerformanceCounter();
        f32 dTInSeconds = 0.001f * dT;
        RunFractalZoomer(&Zoomer, gPrevInput, gInput, dTInSeconds);
        
        char WindowTitleBuffer[1024];
        snprintf(WindowTitleBuffer, sizeof(WindowTitleBuffer), 
                 "Fractal-Zoom: Scale Level: 2^-%.2f, # of Iteration: %d, dT=%.2fms\n", 
                 (f32)Zoomer.Scale, (i32)Zoomer.IterCount, dT);
        SetWindowTextA(Window, WindowTitleBuffer);
        
        SwapBuffers(WindowDC);
        Sleep(3);
    }
    
    return 0;
}