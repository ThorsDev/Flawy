module LitChecker;

import Flawy.MaterialInterface;
#include "Flawy/Sampler.slang"

uniform float4 u_materialParams[3];

FLAWY_SAMPLER2D(s_materialTexture0, 0);

public struct LitChecker : IMaterial
{
    static const ShadingModel kShadingModel = ShadingModel::Lit;
    static const bool kUsesTangentSpaceNormal = false;

    static SurfaceOutput EvaluateSurface(SurfaceInput oInput)
    {
        SurfaceOutput oOutput = MakeDefaultSurface();
        oOutput.vecBaseColor = FLAWY_SAMPLE2D(s_materialTexture0, oInput.vecTexCoord0).rgb * u_materialParams[0].rgb;
        oOutput.fMetallic = u_materialParams[1].x;
        oOutput.fRoughness = u_materialParams[1].y;
        oOutput.vecEmissive = u_materialParams[2].rgb;
        return oOutput;
    }

    static float3 EvaluateWorldPositionOffset(float3 vecWorldPosition, float3 vecWorldNormal)
    {
        return float3(0.0, 0.0, 0.0);
    }
}
