module PassTransform;

import Flawy.MaterialInterface;

public uniform float4x4 u_model;
public uniform float4x4 u_viewProj;

public struct PassVertex
{
    public float3 vecWorldPosition;
    public float3 vecWorldNormal;
    public float4 vecClipPosition;
}

public PassVertex TransformPassVertex<M : IMaterial>(float3 vecLocalPosition, float3 vecLocalNormal)
{
    PassVertex oVertex;
    precise float3 vecWorldPosition = mul(u_model, float4(vecLocalPosition, 1.0)).xyz;
    oVertex.vecWorldNormal = normalize(mul(u_model, float4(vecLocalNormal, 0.0)).xyz);
    vecWorldPosition += M.EvaluateWorldPositionOffset(vecWorldPosition, oVertex.vecWorldNormal);
    precise float4 vecClipPosition = mul(u_viewProj, float4(vecWorldPosition, 1.0));
    oVertex.vecWorldPosition = vecWorldPosition;
    oVertex.vecClipPosition = vecClipPosition;
    return oVertex;
}
