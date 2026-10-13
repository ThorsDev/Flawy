uniform float4x4 u_modelViewProj;

struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR0;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

[shader("vertex")]
VertexOutput VertexMain(VertexInput oInput)
{
    VertexOutput oOutput;
    oOutput.position = mul(u_modelViewProj, float4(oInput.position, 1.0));
    oOutput.color = oInput.color;
    return oOutput;
}

[shader("fragment")]
float4 FragmentMain(VertexOutput oInput) : SV_Target0
{
    return oInput.color;
}
