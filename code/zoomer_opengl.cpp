internal void
UseShader(GLuint Shader)
{
    gCurrentShader = Shader;
    glUseProgram(Shader);
}

internal void
SetUniformFloat(char *Name, f32 Float)
{
    glUniform1f(glGetUniformLocation(gCurrentShader, Name), Float);
}

internal void
SetUniformDouble(char *Name, f64 Double)
{
    glUniform1d(glGetUniformLocation(gCurrentShader, Name), Double);
}

internal void
SetUniformInteger(char *Name, i32 Integer)
{
    glUniform1i(glGetUniformLocation(gCurrentShader, Name), Integer);
}

internal void
SetUniformDouble2(char *Name, f64 Double1, f64 Double2)
{
    glUniform2d(glGetUniformLocation(gCurrentShader, Name), Double1, Double2);
}

