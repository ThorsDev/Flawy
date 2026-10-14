#include "Flawy/Sampler.slang"

uniform float4x4 u_modelViewProj;
uniform float4 u_tint;

FLAWY_SAMPLER2D(s_texColor, 0);

struct VertexInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

[shader("vertex")]
VertexOutput VertexMain(VertexInput oInput)
{
    VertexOutput oOutput;
    oOutput.position = mul(u_modelViewProj, float4(oInput.position, 1.0));
    oOutput.texCoord = oInput.texCoord;
    oOutput.color = oInput.color;
    return oOutput;
}

[shader("fragment")]
float4 FragmentMain(VertexOutput oInput) : SV_Target0
{
    return FLAWY_SAMPLE2D(s_texColor, oInput.texCoord) * oInput.color * u_tint;
}
