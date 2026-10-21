module DepthOnly;

import Flawy.MaterialInterface;
import Flawy.VertexFactory;
import Flawy.PassTransform;

struct VertexOutput
{
    float4 position : SV_Position;
};

[shader("vertex")]
VertexOutput VertexMain<V : IVertexFactory, M : IMaterial>(V.Input oInput)
{
    VertexAttributes oAttributes = V.Fetch(oInput);
    PassVertex oVertex = TransformPassVertex<M>(oAttributes.vecPosition, oAttributes.vecNormal);

    VertexOutput oOutput;
    oOutput.position = oVertex.vecClipPosition;
    return oOutput;
}

[shader("fragment")]
float4 FragmentMain<M : IMaterial>(VertexOutput oInput) : SV_Target0
{
    return float4(0.0, 0.0, 0.0, 1.0);
}
