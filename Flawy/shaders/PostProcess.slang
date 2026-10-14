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

[shader("fragment")]
float4 FragmentMain(VertexOutput oInput) : SV_Target0
{
    float4 vecColor = FLAWY_SAMPLE2D(s_texColor, oInput.texCoord);
    float2 vecOffset = oInput.texCoord - 0.5;
    float fVignette = 1.0 - u_tint.a * dot(vecOffset, vecOffset) * 4.0;
    return float4(vecColor.rgb * u_tint.rgb * fVignette, vecColor.a);
}
