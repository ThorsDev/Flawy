module Lighting;

import Flawy.MaterialInterface;
import Flawy.View;

static const float kPi = 3.14159265;
static const float kMinRoughness = 0.045;
static const float kMinCosine = 0.0001;
static const float kMinDistance = 0.0001;
static const float kMinHalfVectorLengthSquared = 0.00000001;

struct LightingContext
{
    float3 vecNormal;
    float3 vecViewDirection;
    float3 vecDiffuseColor;
    float3 vecSpecularColor;
    float fAlpha;
    float fNdotV;
}

float DistributionGgx(float fNdotH, float fAlpha)
{
    float fAlphaSquared = fAlpha * fAlpha;
    float fDenominator = fNdotH * fNdotH * (fAlphaSquared - 1.0) + 1.0;
    return fAlphaSquared / (kPi * fDenominator * fDenominator);
}

float VisibilitySmithGgxCorrelated(float fNdotV, float fNdotL, float fAlpha)
{
    float fAlphaSquared = fAlpha * fAlpha;
    float fViewTerm = fNdotL * sqrt(fNdotV * fNdotV * (1.0 - fAlphaSquared) + fAlphaSquared);
    float fLightTerm = fNdotV * sqrt(fNdotL * fNdotL * (1.0 - fAlphaSquared) + fAlphaSquared);
    return 0.5 / (fViewTerm + fLightTerm);
}

float3 FresnelSchlick(float3 vecF0, float fVdotH)
{
    return vecF0 + (1.0 - vecF0) * pow(1.0 - fVdotH, 5.0);
}

float3 EvaluateLight(LightingContext oContext, float3 vecLightDirection, float3 vecRadiance)
{
    float fNdotL = saturate(dot(oContext.vecNormal, vecLightDirection));
    float3 vecHalfSum = oContext.vecViewDirection + vecLightDirection;
    float3 vecHalf = vecHalfSum * rsqrt(max(dot(vecHalfSum, vecHalfSum), kMinHalfVectorLengthSquared));
    float fNdotH = saturate(dot(oContext.vecNormal, vecHalf));
    float fVdotH = saturate(dot(oContext.vecViewDirection, vecHalf));

    float3 vecFresnel = FresnelSchlick(oContext.vecSpecularColor, fVdotH);
    float3 vecSpecular = DistributionGgx(fNdotH, oContext.fAlpha) * VisibilitySmithGgxCorrelated(oContext.fNdotV, max(fNdotL, kMinCosine), oContext.fAlpha) * vecFresnel;
    float3 vecDiffuse = (1.0 - vecFresnel) * oContext.vecDiffuseColor / kPi;
    return (vecDiffuse + vecSpecular) * vecRadiance * fNdotL;
}

float GetPointLightAttenuation(float fDistance, float fRadius)
{
    float fRatio = fDistance / fRadius;
    float fWindow = saturate(1.0 - fRatio * fRatio * fRatio * fRatio);
    return fWindow * fWindow / (fDistance * fDistance + 1.0);
}

public float3 ShadeLit(SurfaceOutput oSurface, float3 vecWorldNormal, float3 vecViewDirection, float3 vecWorldPosition)
{
    float fRoughness = clamp(oSurface.fRoughness, kMinRoughness, 1.0);

    LightingContext oContext;
    oContext.vecNormal = vecWorldNormal;
    oContext.vecViewDirection = vecViewDirection;
    oContext.vecDiffuseColor = oSurface.vecBaseColor * (1.0 - oSurface.fMetallic);
    oContext.vecSpecularColor = lerp(float3(0.04, 0.04, 0.04), oSurface.vecBaseColor, oSurface.fMetallic);
    oContext.fAlpha = fRoughness * fRoughness;
    oContext.fNdotV = max(dot(vecWorldNormal, vecViewDirection), kMinCosine);

    float3 vecColor = float3(0.0, 0.0, 0.0);
    if (IsDirectionalLightEnabled())
    {
        vecColor += EvaluateLight(oContext, -GetDirectionalLightDirection(), GetDirectionalLightRadiance());
    }

    [ForceUnroll]
    for (int iLight = 0; iLight < kMaxPointLights; ++iLight)
    {
        float3 vecToLight = GetPointLightPosition(iLight) - vecWorldPosition;
        float fDistance = length(vecToLight);
        float fAttenuation = GetPointLightAttenuation(fDistance, GetPointLightRadius(iLight));
        vecColor += EvaluateLight(oContext, vecToLight / max(fDistance, kMinDistance), GetPointLightRadiance(iLight) * fAttenuation);
    }

    float3 vecAmbient = lerp(GetGroundAmbient(), GetSkyAmbient(), vecWorldNormal.y * 0.5 + 0.5);
    vecColor += vecAmbient * oSurface.vecBaseColor * oSurface.fAmbientOcclusion;
    return vecColor + oSurface.vecEmissive;
}
