#include <windows.h>
#include <windowsx.h>
#include "ch_gl.h"

#include "kernel.h"
#include "zoomer.cpp"

#include "win32_kernel.h"

//TODO(chen): double-buffered input state
global_variable input gPrevInput;
global_variable input gInput;
global_variable b32 gAppIsRunning;
global_variable int gWindowWidth;
global_variable int gWindowHeight;

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
    gWindowWidth = 1080;
    gWindowHeight = 724;
    HWND Window = Win32CreateWindow(CurrentInstance, 
                                    gWindowWidth, gWindowHeight,
                                    "Fractal-Zoom", "Fractal-Zoom windowclass",
                                    Win32WindowCallback);
    HDC WindowDC = GetDC(Window);
    Win32InitializeOpengl(WindowDC, 4, 0);
    wglSwapInterval(1);
    LoadGLFunctions(Win32GetOpenglFunction);
    
    zoomer Zoomer = {};
    
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
        
        RunFractalZoomer(&Zoomer, gPrevInput, gInput, gWindowWidth, gWindowHeight);
        
        SwapBuffers(WindowDC);
        Sleep(5);
    }
    
    return 0;
}