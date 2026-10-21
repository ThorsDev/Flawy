module CheckerUnlit;

import Flawy.MaterialInterface;
#include "Flawy/Sampler.slang"

uniform float4 u_materialParams[1];

FLAWY_SAMPLER2D(s_materialTexture0, 0);

public struct CheckerUnlit : IMaterial
{
    static const ShadingModel kShadingModel = ShadingModel::Unlit;
    static const bool kUsesTangentSpaceNormal = false;

    static SurfaceOutput EvaluateSurface(SurfaceInput oInput)
    {
        SurfaceOutput oOutput = MakeDefaultSurface();
        oOutput.vecBaseColor = FLAWY_SAMPLE2D(s_materialTexture0, oInput.vecTexCoord0).rgb * u_materialParams[0].rgb * oInput.vecVertexColor.rgb;
        return oOutput;
    }

    static float3 EvaluateWorldPositionOffset(float3 vecWorldPosition, float3 vecWorldNormal)
    {
        return float3(0.0, 0.0, 0.0);
    }
}
