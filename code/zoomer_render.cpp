char *QuadVShaderSource = R"(
#version 400

in layout(location = 0) vec3 P;
out vec2 FragP;

void main()
{
FragP = vec2(P.x, P.y);
gl_Position = vec4(P, 1.0);
}

)";

char *MandelbrotFShaderSource = R"(
#version 400

uniform float AspectRatio;

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
dvec2 C = dvec2(FragP.x * AspectRatio, FragP.y) / pow(2.0, float(ZoomScale)) - ZoomP;

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

char *BlitFShaderSource = R"(
#version 400

uniform sampler2D Tex;

 in vec2 FragP;
out vec3 FragColor;

void main()
{
vec2 TexCoord = 0.5 * FragP + 0.5;
FragColor = texture(Tex, TexCoord).rgb;
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

internal GLuint 
MakeShader(char *VShaderSource, char *FShaderSource)
{
    GLuint VShader = CompileShader(GL_VERTEX_SHADER, VShaderSource);
    GLuint FShader = CompileShader(GL_FRAGMENT_SHADER, FShaderSource);
    
    GLuint Shader = glCreateProgram();
    glAttachShader(Shader, VShader);
    glAttachShader(Shader, FShader);
    glLinkProgram(Shader);
    
    GLint ProgramIsLinked;
    glGetProgramiv(Shader, GL_LINK_STATUS, &ProgramIsLinked);
    if (ProgramIsLinked != GL_TRUE)
    {
        GLchar ErrorMessage[1024];
        glGetProgramInfoLog(Shader, sizeof(ErrorMessage), 
                            0, ErrorMessage);
        ASSERT(!"Failed to link shader program");
    }
    
    return Shader;
}

internal rs
InitRS()
{
    rs RS = {};
    
    RS.MandelbrotShader = MakeShader(QuadVShaderSource,
                                     MandelbrotFShaderSource);
    RS.BlitShader = MakeShader(QuadVShaderSource,
                               BlitFShaderSource);
    
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
    
    glGenVertexArrays(1, &RS.QuadVAO);
    glBindVertexArray(RS.QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glBindVertexArray(0);
    
    for (int FramebufferIndex = 0; 
         FramebufferIndex < ARRAY_COUNT(RS.Framebuffers);
         ++FramebufferIndex)
    {
        framebuffer *FBO = RS.Framebuffers + FramebufferIndex;
        glGenFramebuffers(1, &FBO->Handle);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO->Handle);
        
        glGenTextures(1, &FBO->TexHandle);
        glBindTexture(GL_TEXTURE_2D, FBO->TexHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gWindowWidth, gWindowHeight,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                               GL_TEXTURE_2D, FBO->TexHandle, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        glGenRenderbuffers(1, &FBO->RBOHandle);
        glBindRenderbuffer(GL_RENDERBUFFER, FBO->RBOHandle);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 
                              gWindowWidth, gWindowHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, FBO->RBOHandle);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        
        GLenum FramebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        ASSERT(FramebufferStatus == GL_FRAMEBUFFER_COMPLETE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    return RS;
}

internal void
RenderMandelbrot(zoomer *Zoomer)
{
    rs RS = Zoomer->RS;
    
    UseShader(RS.MandelbrotShader);
    SetUniformFloat("AspectRatio", (f32)gWindowWidth / (f32)gWindowHeight);
    SetUniformDouble("ZoomScale", Zoomer->Scale);
    SetUniformInteger("IterCount", (i32)Zoomer->IterCount);
    
    v2d ZoomP = Zoomer->P;
    if (Zoomer->IsMoving)
    {
        ZoomP = Zoomer->P + Zoomer->dP;
    }
    SetUniformDouble2("ZoomP", ZoomP.X, ZoomP.Y);
    
    glBindFramebuffer(GL_FRAMEBUFFER, RS.Framebuffers[0].Handle);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    glBindVertexArray(RS.QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    UseShader(RS.BlitShader);
    glBindTexture(GL_TEXTURE_2D, RS.Framebuffers[0].TexHandle);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    glBindVertexArray(RS.QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
