#include "zoomer.h"

char *VShaderSource = R"(
#version 400

in layout(location = 0) vec3 P;
out vec3 FragP;

void main()
{
FragP = P;
gl_Position = vec4(P, 1.0);
}

)";

char *FShaderSource = R"(
#version 400

in vec3 FragP;
out vec3 FragColor;

void main()
{
FragColor = FragP;
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

internal void
RunFractalZoomer(zoomer *Zoomer, input Input)
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
        
        Zoomer->IsInitialized = true;
    }
    
    glUseProgram(Zoomer->Shader);
    glBindVertexArray(Zoomer->QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
