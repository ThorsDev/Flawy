module MaterialInterface;

public enum ShadingModel
{
    Unlit,
    Lit
}

public struct SurfaceInput
{
    public float3 vecWorldPosition;
    public float3 vecWorldNormal;
    public float4 vecWorldTangent;
    public float3 vecViewDirection;
    public float2 vecTexCoord0;
    public float4 vecVertexColor;
    public float fTimeSeconds;
}

public struct SurfaceOutput
{
    public float3 vecBaseColor;
    public float3 vecNormal;
    public float fMetallic;
    public float fRoughness;
    public float fAmbientOcclusion;
    public float3 vecEmissive;
    public float fOpacity;
}

public interface IMaterial
{
    static const ShadingModel kShadingModel;
    static const bool kUsesTangentSpaceNormal;
    static SurfaceOutput EvaluateSurface(SurfaceInput oInput);
    static float3 EvaluateWorldPositionOffset(float3 vecWorldPosition, float3 vecWorldNormal);
}

public SurfaceOutput MakeDefaultSurface()
{
    SurfaceOutput oOutput;
    oOutput.vecBaseColor = float3(1.0, 1.0, 1.0);
    oOutput.vecNormal = float3(0.0, 0.0, 1.0);
    oOutput.fMetallic = 0.0;
    oOutput.fRoughness = 0.5;
    oOutput.fAmbientOcclusion = 1.0;
    oOutput.vecEmissive = float3(0.0, 0.0, 0.0);
    oOutput.fOpacity = 1.0;
    return oOutput;
}
