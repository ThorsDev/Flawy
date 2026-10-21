module VertexFactory;

public struct VertexAttributes
{
    public float3 vecPosition;
    public float3 vecNormal;
    public float4 vecTangent;
    public float2 vecTexCoord0;
    public float4 vecColor;
}

public interface IVertexFactory
{
    associatedtype Input;
    static const bool kHasTangent;
    static VertexAttributes Fetch(Input oInput);
}

public struct StaticMeshInput
{
    public float3 position : POSITION;
    public float3 normal : NORMAL;
    public float2 texCoord : TEXCOORD0;
    public float4 color : COLOR0;
}

public struct StaticMeshVertexFactory : IVertexFactory
{
    public typealias Input = StaticMeshInput;
    public static const bool kHasTangent = false;

    public static VertexAttributes Fetch(StaticMeshInput oInput)
    {
        VertexAttributes oAttributes;
        oAttributes.vecPosition = oInput.position;
        oAttributes.vecNormal = oInput.normal;
        oAttributes.vecTangent = float4(1.0, 0.0, 0.0, 1.0);
        oAttributes.vecTexCoord0 = oInput.texCoord;
        oAttributes.vecColor = oInput.color;
        return oAttributes;
    }
}

public struct StaticMeshTangentInput
{
    public float3 position : POSITION;
    public float3 normal : NORMAL;
    public float4 tangent : TANGENT;
    public float2 texCoord : TEXCOORD0;
    public float4 color : COLOR0;
}

public struct StaticMeshTangentVertexFactory : IVertexFactory
{
    public typealias Input = StaticMeshTangentInput;
    public static const bool kHasTangent = true;

    public static VertexAttributes Fetch(StaticMeshTangentInput oInput)
    {
        VertexAttributes oAttributes;
        oAttributes.vecPosition = oInput.position;
        oAttributes.vecNormal = oInput.normal;
        oAttributes.vecTangent = oInput.tangent;
        oAttributes.vecTexCoord0 = oInput.texCoord;
        oAttributes.vecColor = oInput.color;
        return oAttributes;
    }
}
