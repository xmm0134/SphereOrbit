cbuffer globals : register(b0)
{
    float4x4 ViewProj;
    float4x4 World;
}

struct VSInput
{
    float4 Position : POSITION;
    float3 Normal : NORMAL;
};
struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 WorldPosition : TEXCOORD0;
    float3 Normal : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;
    float4x4 MVP = mul(ViewProj, World);
    output.Position = mul(MVP, input.Position);
    output.Normal = input.Normal;
    output.WorldPosition = mul(World, input.Position);
    
    return output;
}