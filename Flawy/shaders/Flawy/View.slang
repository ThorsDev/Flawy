module View;

public static const int kMaxPointLights = 4;

uniform float4 u_viewParams[2];
uniform float4 u_lightParams[12];

public float3 GetCameraPosition()
{
    return u_viewParams[0].xyz;
}

public float GetTimeSeconds()
{
    return u_viewParams[0].w;
}

public float2 GetViewportSize()
{
    return u_viewParams[1].xy;
}

public float3 GetDirectionalLightDirection()
{
    return u_lightParams[0].xyz;
}

public bool IsDirectionalLightEnabled()
{
    return u_lightParams[0].w > 0.5;
}

public float3 GetDirectionalLightRadiance()
{
    return u_lightParams[1].rgb;
}

public float3 GetSkyAmbient()
{
    return u_lightParams[2].rgb;
}

public float3 GetGroundAmbient()
{
    return u_lightParams[3].rgb;
}

public float3 GetPointLightPosition(int iIndex)
{
    return u_lightParams[4 + 2 * iIndex].xyz;
}

public float GetPointLightRadius(int iIndex)
{
    return u_lightParams[4 + 2 * iIndex].w;
}

public float3 GetPointLightRadiance(int iIndex)
{
    return u_lightParams[5 + 2 * iIndex].rgb;
}
