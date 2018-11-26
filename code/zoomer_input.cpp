inline v2d
NormalizeMouseP(v2 MouseP, f32 AspectRatio)
{
    v2d Result = {};
    Result.X = AspectRatio * (2.0 * (f64)MouseP.X - 1.0);
    Result.Y = 2.0 * (1.0 - (f64)MouseP.Y) - 1.0;
    return Result;
}

internal void
HandleInput(zoomer *Zoomer, input PrevInput, input Input, f32 dT)
{
    zoomer OldZoomer = *Zoomer;
    
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
    
    f32 AspectRatio = (f32)gWindowWidth / (f32)gWindowHeight;
    
    if (!PrevInput.MouseIsDown && Input.MouseIsDown)
    {
        Zoomer->IsMoving = true;
        Zoomer->StartP = NormalizeMouseP(Input.MouseP, AspectRatio);
    }
    
    if (Zoomer->IsMoving)
    {
        v2d InputP = NormalizeMouseP(Input.MouseP, AspectRatio);
        Zoomer->dP = (1.0 / pow(2.0, Zoomer->Scale)) * (InputP - Zoomer->StartP);
        
        if (!Input.MouseIsDown)
        {
            Zoomer->P = Zoomer->P + Zoomer->dP;
            Zoomer->IsMoving = false;
        }
    }
    
    Zoomer->IsUpdated = memcmp(&OldZoomer, Zoomer, sizeof(zoomer)) != 0;
}
