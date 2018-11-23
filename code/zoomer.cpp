/*
TODO(chen):

1. grill fractal zoomer formulas
2. port to linux

*/
#include "zoomer.h"

char *VShaderSource = R"(
#version 400

uniform float AspectRatio;

in layout(location = 0) vec3 P;
out vec2 FragP;

void main()
{
FragP = vec2(P.x * AspectRatio, P.y);
gl_Position = vec4(P, 1.0);
}

)";

char *FShaderSource = R"(
#version 400

uniform dvec2 ZoomP;
uniform double ZoomScale;  
uniform int IterCount;

in vec2 FragP;
out vec3 FragColor;

dvec2 ComplexSq(in dvec2 p)
{
return dvec2(p.x*p.x - p.y*p.y, 2.0*p.x*p.y);
}

vec3 palette(float t)
{
vec3[] colors = vec3[](

vec3(0.1,0.4, 0.7),
vec3(1, 0.7, 0),
vec3(1, 0, 1),

vec3(0.1,0.4, 0.7),
vec3(1, 0.7, 0),
vec3(1, 0, 1),

vec3(0, 0, 0.4)
);

int color_count = colors.length();
int real_count = color_count - 1;
for (int i = 1; i <= real_count; ++i)
{
if (t <= float(i) / float(real_count))
{
return mix(colors[i-1], colors[i], float(real_count) * (t - float(i-1) / float(real_count)));
}
}

}

void main()
{
dvec2 Z = dvec2(0);
dvec2 C = dvec2(FragP) / pow(2.0, float(ZoomScale)) - ZoomP;

int Iter;
for (Iter = 0; Iter < IterCount; ++Iter)
{
Z = ComplexSq(Z) + C;
if (dot(Z, Z) > 4.0) break;
}

 float t = float(Iter) / float(IterCount);
 
FragColor = palette(t);
}

)";

internal GLuint
CompileShader(GLenum Type, char *Source)
{
    GLuint Shader = glCreateShader(Type);
    glShaderSource(Shader, 1, &Source, 0);
    glCompileShader(Shader);
    
    GLint ShaderIsCompiled;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &ShaderIsCompiled);
    if (ShaderIsCompiled != GL_TRUE)
    {
        GLchar ErrorMessage[1024];
        glGetShaderInfoLog(Shader, sizeof(ErrorMessage), 0, ErrorMessage);
        ASSERT(!"Failed to compile shader");
    }
    
    return Shader;
}

inline v2d
NormalizeMouseP(v2 MouseP, f32 AspectRatio)
{
    v2d Result = {};
    Result.X = AspectRatio * (2.0 * (f64)MouseP.X - 1.0);
    Result.Y = 2.0 * (1.0 - (f64)MouseP.Y) - 1.0;
    return Result;
}

internal void
RunFractalZoomer(zoomer *Zoomer, input PrevInput, input Input, f32 dT,
                 int WindowWidth, int WindowHeight)
{
    if (!Zoomer->IsInitialized)
    {
        GLuint VShader = CompileShader(GL_VERTEX_SHADER, VShaderSource);
        GLuint FShader = CompileShader(GL_FRAGMENT_SHADER, FShaderSource);
        
        Zoomer->Shader = glCreateProgram();
        glAttachShader(Zoomer->Shader, VShader);
        glAttachShader(Zoomer->Shader, FShader);
        glLinkProgram(Zoomer->Shader);
        
        GLint ProgramIsLinked;
        glGetProgramiv(Zoomer->Shader, GL_LINK_STATUS, &ProgramIsLinked);
        if (ProgramIsLinked != GL_TRUE)
        {
            GLchar ErrorMessage[1024];
            glGetProgramInfoLog(Zoomer->Shader, sizeof(ErrorMessage), 
                                0, ErrorMessage);
            ASSERT(!"Failed to link shader program");
        }
        
        f32 QuadVertices[18] = {
            -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f,
        };
        
        GLuint VBO = 0;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), QuadVertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glGenVertexArrays(1, &Zoomer->QuadVAO);
        glBindVertexArray(Zoomer->QuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glBindVertexArray(0);
        
        Zoomer->P = {0.5, 0.0};
        Zoomer->Scale = 1.0;
        Zoomer->IterCount = 200;
        
        Zoomer->IsInitialized = true;
    }
    
    f32 AspectRatio = (f32)WindowWidth / (f32)WindowHeight;
    
    if (Input.SpaceIsDown)
    {
        if (Input.ShiftIsDown)
        {
            Zoomer->Scale -= 3.0f * dT;
        }
        else
        {
            Zoomer->Scale += 3.0f * dT;
        }
    }
    
    if (Input.UpArrowIsDown)
    {
        Zoomer->IterCount = Min(1000.0f, Zoomer->IterCount + 200.0f * dT);
    }
    if (Input.DownArrowIsDown)
    {
        Zoomer->IterCount = Max(10.0f, Zoomer->IterCount - 200.0f * dT);
    }
    
    
    if (!PrevInput.MouseIsDown && Input.MouseIsDown)
    {
        Zoomer->IsMoving = true;
        Zoomer->StartP = NormalizeMouseP(Input.MouseP, AspectRatio);
    }
    
    v2d dP = {};
    if (Zoomer->IsMoving)
    {
        v2d InputP = NormalizeMouseP(Input.MouseP, AspectRatio);
        dP = (1.0 / pow(2.0, Zoomer->Scale)) * (InputP - Zoomer->StartP);
        
        if (!Input.MouseIsDown)
        {
            Zoomer->P = Zoomer->P + dP;
            Zoomer->IsMoving = false;
        }
    }
    
    glUseProgram(Zoomer->Shader);
    glUniform1f(glGetUniformLocation(Zoomer->Shader, "AspectRatio"),
                AspectRatio);
    glUniform1d(glGetUniformLocation(Zoomer->Shader, "ZoomScale"),
                Zoomer->Scale);
    glUniform1i(glGetUniformLocation(Zoomer->Shader, "IterCount"),
                (i32)Zoomer->IterCount);
    
    if (!Zoomer->IsMoving)
    {
        glUniform2d(glGetUniformLocation(Zoomer->Shader, "ZoomP"),
                    Zoomer->P.X, Zoomer->P.Y);
    }
    else
    {
        v2d TentativeP = Zoomer->P + dP;
        glUniform2d(glGetUniformLocation(Zoomer->Shader, "ZoomP"),
                    TentativeP.X, TentativeP.Y);
    }
    
    glViewport(0, 0, WindowWidth, WindowHeight);
    glBindVertexArray(Zoomer->QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
