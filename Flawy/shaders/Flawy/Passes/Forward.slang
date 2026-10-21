module Forward;

import Flawy.MaterialInterface;
import Flawy.VertexFactory;
import Flawy.View;
import Flawy.Lighting;
import Flawy.PassTransform;

struct VertexOutput
{
    float4 position : SV_Position;
    float3 worldPosition : TEXCOORD1;
    float3 worldNormal : NORMAL;
    float4 worldTangent : TANGENT;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

float GetModelHandedness()
{
    float fDeterminant = dot(cross(u_model[0].xyz, u_model[1].xyz), u_model[2].xyz);
    return fDeterminant < 0.0 ? -1.0 : 1.0;
}

float3 ResolveShadingNormal(float3 vecWorldNormal, float4 vecWorldTangent, float3 vecSurfaceNormal)
{
    float3 vecTangent = normalize(vecWorldTangent.xyz - vecWorldNormal * dot(vecWorldNormal, vecWorldTangent.xyz));
    float3 vecBitangent = cross(vecWorldNormal, vecTangent) * vecWorldTangent.w;
    return normalize(vecTangent * vecSurfaceNormal.x + vecBitangent * vecSurfaceNormal.y + vecWorldNormal * vecSurfaceNormal.z);
}

[shader("vertex")]
VertexOutput VertexMain<V : IVertexFactory, M : IMaterial>(V.Input oInput)
{
    VertexAttributes oAttributes = V.Fetch(oInput);
    PassVertex oVertex = TransformPassVertex<M>(oAttributes.vecPosition, oAttributes.vecNormal);

    VertexOutput oOutput;
    oOutput.position = oVertex.vecClipPosition;
    oOutput.worldPosition = oVertex.vecWorldPosition;
    oOutput.worldNormal = oVertex.vecWorldNormal;
    oOutput.worldTangent = V.kHasTangent ? float4(normalize(mul(u_model, float4(oAttributes.vecTangent.xyz, 0.0)).xyz), oAttributes.vecTangent.w * GetModelHandedness()) : float4(0.0, 0.0, 0.0, 0.0);
    oOutput.texCoord = oAttributes.vecTexCoord0;
    oOutput.color = oAttributes.vecColor;
    return oOutput;
}

[shader("fragment")]
float4 FragmentMain<M : IMaterial>(VertexOutput oInput) : SV_Target0
{
    float3 vecWorldNormal = normalize(oInput.worldNormal);

    SurfaceInput oSurfaceInput;
    oSurfaceInput.vecWorldPosition = oInput.worldPosition;
    oSurfaceInput.vecWorldNormal = vecWorldNormal;
    oSurfaceInput.vecWorldTangent = oInput.worldTangent;
    oSurfaceInput.vecViewDirection = normalize(GetCameraPosition() - oInput.worldPosition);
    oSurfaceInput.vecTexCoord0 = oInput.texCoord;
    oSurfaceInput.vecVertexColor = oInput.color;
    oSurfaceInput.fTimeSeconds = GetTimeSeconds();

    SurfaceOutput oSurface = M.EvaluateSurface(oSurfaceInput);
    if (M.kShadingModel == ShadingModel::Unlit)
    {
        return float4(oSurface.vecBaseColor + oSurface.vecEmissive, oSurface.fOpacity);
    }

    float3 vecShadingNormal = vecWorldNormal;
    if (M.kUsesTangentSpaceNormal && length(oInput.worldTangent.xyz) > 0.0)
    {
        vecShadingNormal = ResolveShadingNormal(vecWorldNormal, oInput.worldTangent, oSurface.vecNormal);
    }
    return float4(ShadeLit(oSurface, vecShadingNormal, oSurfaceInput.vecViewDirection, oInput.worldPosition), oSurface.fOpacity);
}
