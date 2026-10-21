#include "Flawy/Sampler.slang"

uniform float4 u_tint;

FLAWY_SAMPLER2D(s_texColor, 0);

struct VertexInput
{
    float2 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

[shader("vertex")]
VertexOutput VertexMain(VertexInput oInput)
{
    VertexOutput oOutput;
    oOutput.position = float4(oInput.position, 0.0, 1.0);
    oOutput.texCoord = oInput.texCoord;
    return oOutput;
}

static const float kDisplayGamma = 2.2;

float3 TonemapAcesFitted(float3 vecColor)
{
    float3 vecNumerator = vecColor * (2.51 * vecColor + 0.03);
    float3 vecDenominator = vecColor * (2.43 * vecColor + 0.59) + 0.14;
    return saturate(vecNumerator / vecDenominator);
}

[shader("fragment")]
float4 FragmentMain(VertexOutput oInput) : SV_Target0
{
    float4 vecColor = FLAWY_SAMPLE2D(s_texColor, oInput.texCoord);
    float3 vecDisplay = pow(TonemapAcesFitted(vecColor.rgb), 1.0 / kDisplayGamma);
    float2 vecOffset = oInput.texCoord - 0.5;
    float fVignette = 1.0 - u_tint.a * dot(vecOffset, vecOffset) * 4.0;
    return float4(vecDisplay * u_tint.rgb * fVignette, vecColor.a);
}
