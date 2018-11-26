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
for (int i = 1; i < real_count; ++i)
{
if (t <= float(i) / float(real_count))
{
return mix(colors[i-1], colors[i], float(real_count) * (t - float(i-1) / float(real_count)));
}
}

return mix(colors[real_count-1], colors[real_count], float(real_count) * (t - float(real_count-1) / float(real_count)));
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

internal rs
InitRS()
{
    rs Result = {};
    
    GLuint VShader = CompileShader(GL_VERTEX_SHADER, VShaderSource);
    GLuint FShader = CompileShader(GL_FRAGMENT_SHADER, FShaderSource);
    
    Result.Shader = glCreateProgram();
    glAttachShader(Result.Shader, VShader);
    glAttachShader(Result.Shader, FShader);
    glLinkProgram(Result.Shader);
    
    GLint ProgramIsLinked;
    glGetProgramiv(Result.Shader, GL_LINK_STATUS, &ProgramIsLinked);
    if (ProgramIsLinked != GL_TRUE)
    {
        GLchar ErrorMessage[1024];
        glGetProgramInfoLog(Result.Shader, sizeof(ErrorMessage), 
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
    
    glGenVertexArrays(1, &Result.QuadVAO);
    glBindVertexArray(Result.QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glBindVertexArray(0);
    
    
    
    return Result;
}

internal void
RedrawMandelbrot(zoomer *Zoomer)
{
    rs RS = Zoomer->RS;
    
    glUseProgram(RS.Shader);
    glUniform1f(glGetUniformLocation(RS.Shader, "AspectRatio"),
                (f32)gWindowWidth / (f32)gWindowHeight);
    glUniform1d(glGetUniformLocation(RS.Shader, "ZoomScale"),
                Zoomer->Scale);
    glUniform1i(glGetUniformLocation(RS.Shader, "IterCount"),
                (i32)Zoomer->IterCount);
    if (!Zoomer->IsMoving)
    {
        glUniform2d(glGetUniformLocation(RS.Shader, "ZoomP"),
                    Zoomer->P.X, Zoomer->P.Y);
    }
    else
    {
        v2d TentativeP = Zoomer->P + Zoomer->dP;
        glUniform2d(glGetUniformLocation(RS.Shader, "ZoomP"),
                    TentativeP.X, TentativeP.Y);
    }
    
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    glBindVertexArray(RS.QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

internal void
ProgressivelyRenderMandelbrot(zoomer *Zoomer)
{
    RedrawMandelbrot(Zoomer);
}
