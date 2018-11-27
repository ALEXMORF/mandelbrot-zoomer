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

uniform float uAspectRatio;
uniform float uTime;

uniform dvec2 uZoomP;
uniform double uZoomScale;  
uniform int uIterCount;

//NOTE(chen): for blending progressively rendered results
uniform float uPrevWeight;
uniform sampler2D uPrevFrame;

in vec2 FragP;
out vec3 FragColor;

 float Hash(in vec2 P)
{
return fract(34135.13*sin(dot(P, vec2(83.13, 951.31))));
}

vec2 Jitter(in vec2 Seed)
{
vec2 Res;
Res.x = Hash(275.14*Seed + 8.13*uTime);
Res.y = Hash(975.14*Seed + 71.49*uTime);
return 2.0 * Res - 1.0;
}

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
vec2 UV = FragP + 0.5 * Jitter(FragP) * (vec2(1.0) / textureSize(uPrevFrame, 0));

dvec2 Z = dvec2(0);
dvec2 C = dvec2(UV.x * uAspectRatio, UV.y) / pow(2.0, float(uZoomScale)) - uZoomP;

int Iter;
for (Iter = 0; Iter < uIterCount; ++Iter)
{
Z = ComplexSq(Z) + C;
if (dot(Z, Z) > 4.0) break;
}

 float t = float(Iter) / float(uIterCount);
  vec3 Color = palette(t);
  
vec2 TexCoord = 0.5 * FragP + 0.5;
vec3 PrevFragColor = texture(uPrevFrame, TexCoord).rgb;

FragColor = uPrevWeight * PrevFragColor + (1.0 - uPrevWeight) * Color;
}

)";

char *BlitFShaderSource = R"(
#version 400

uniform sampler2D uTex;

 in vec2 FragP;
out vec3 FragColor;

void main()
{
vec2 TexCoord = 0.5 * FragP + 0.5;
FragColor = texture(uTex, TexCoord).rgb;
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

internal framebuffer
MakeFramebuffer(int Width, int Height)
{
    framebuffer FBO = {};
    
    FBO.Width = gWindowWidth;
    FBO.Height = gWindowHeight;
    
    glGenFramebuffers(1, &FBO.Handle);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO.Handle);
    
    glGenTextures(1, &FBO.TexHandle);
    glBindTexture(GL_TEXTURE_2D, FBO.TexHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gWindowWidth, gWindowHeight,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                           GL_TEXTURE_2D, FBO.TexHandle, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glGenRenderbuffers(1, &FBO.RBOHandle);
    glBindRenderbuffer(GL_RENDERBUFFER, FBO.RBOHandle);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 
                          gWindowWidth, gWindowHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, FBO.RBOHandle);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    
    GLenum FramebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    ASSERT(FramebufferStatus == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return FBO;
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
        RS.Framebuffers[FramebufferIndex] = MakeFramebuffer(gWindowWidth, gWindowHeight);
    }
    
    return RS;
}

internal void
RenderMandelbrot(zoomer *Zoomer)
{
    rs *RS = &Zoomer->RS;
    
    for (int FramebufferIndex = 0; 
         FramebufferIndex < ARRAY_COUNT(RS->Framebuffers);
         ++FramebufferIndex)
    {
        framebuffer *FBO = RS->Framebuffers + FramebufferIndex;
        if (FBO->Width != gWindowWidth || FBO->Height != gWindowHeight)
        {
            glDeleteTextures(1, &FBO->TexHandle);
            glDeleteRenderbuffers(1, &FBO->RBOHandle);
            glDeleteFramebuffers(1, &FBO->Handle);
            
            *FBO = MakeFramebuffer(gWindowWidth, gWindowHeight);
            Zoomer->IsUpdated = true;
        }
    }
    
    if (Zoomer->IsUpdated)
    {
        RS->FrameIndex = 0;
    }
    
    UseShader(RS->MandelbrotShader);
    SetUniformFloat("uAspectRatio", (f32)gWindowWidth / (f32)gWindowHeight);
    SetUniformFloat("uTime", Zoomer->Time);
    SetUniformDouble("uZoomScale", Zoomer->Scale);
    SetUniformInteger("uIterCount", (i32)Zoomer->IterCount);
    v2d ZoomP = Zoomer->P;
    if (Zoomer->IsMoving)
    {
        ZoomP = Zoomer->P + Zoomer->dP;
    }
    SetUniformDouble2("uZoomP", ZoomP.X, ZoomP.Y);
    
    int FrameCount = RS->FrameIndex + 1;
    SetUniformFloat("uPrevWeight", (f32)RS->FrameIndex / (f32)FrameCount);
    
    int LastFBOIndex = RS->CurrentFBOIndex;
    RS->CurrentFBOIndex = (RS->CurrentFBOIndex + 1) % ARRAY_COUNT(RS->Framebuffers);
    
    glBindTexture(GL_TEXTURE_2D, RS->Framebuffers[LastFBOIndex].TexHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, RS->Framebuffers[RS->CurrentFBOIndex].Handle);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    glBindVertexArray(RS->QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    UseShader(RS->BlitShader);
    glBindTexture(GL_TEXTURE_2D, RS->Framebuffers[RS->CurrentFBOIndex].TexHandle);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
    glBindVertexArray(RS->QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    RS->FrameIndex += 1;
}
