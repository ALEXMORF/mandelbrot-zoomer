@echo off

IF NOT EXIST ..\build mkdir ..\build

pushd ..\build

set CompilerFlags=-nologo -FC -Fewin32_mandelbrot.exe -Z7 -W4 -WX -wd4100 -wd4505 -wd4189 -wd4201
set LinkerFlags=Gdi32.lib User32.lib Opengl32.lib
cl %CompilerFlags% ..\code\win32_main.cpp %LinkerFlags%

popd