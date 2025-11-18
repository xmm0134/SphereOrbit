
struct tLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float3 LightDirection;
    float Padding;
};


struct tMaterial
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float4 Padding;
};

tLight ComputeDirectionalLight(tMaterial mat, tLight SceneLighting, float3 normal, float3 Position, float3 EyePosition)
{
    tLight light = (tLight) 0;
    float3 LightVec = normalize(-SceneLighting.LightDirection); // Flip vector 
    light.Ambient = SceneLighting.Ambient * mat.Ambient;
    float DiffuseFactor = max(dot(LightVec, normal),0.0f);
    light.Diffuse = mat.Diffuse * DiffuseFactor * SceneLighting.Diffuse;
    
    if (DiffuseFactor > 0.0f)
    {
        float3 r = reflect(-LightVec, normal);
        float3 p = normalize(EyePosition - Position);

        float SpecFactor = pow(max(dot(r, p), 0.0f), mat.Specular.w);
        light.Specular = mat.Specular * SpecFactor * SceneLighting.Specular;
    }
    
    return light;
}


cbuffer Lighting : register(b0)
{
    float4 LAmbient;
    float4 LDiffuse;
    float4 LSpecular;
    float3 LightDir;
    float3 LCameraPos;
    float2 LPad;
}

cbuffer Material : register(b1)
{
    float4 MAmbient;
    float4 MDiffuse;
    float4 MSpecular;
    float4 MPad;
}

cbuffer PointLight : register(b2)
{
    float4 PAmbient;
    float4 PDiffuse;
    float4 Pspecular;
    float3 PPosition;
    float3 Attenuation;
    float PRange;
    float Padding;
}

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 WorldPosition : TEXCOORD0;
    float3 Normal : TEXCOORD1;
};

float4 main(PSInput input) : SV_TARGET
{
    tLight SceneLighting = (tLight) 0;
    SceneLighting.Ambient = LAmbient;
    SceneLighting.Diffuse = LDiffuse;
    SceneLighting.Specular = LSpecular;
    SceneLighting.LightDirection = LightDir;
    
    
    tMaterial mat = (tMaterial) 0;
    mat.Ambient = MAmbient;
    mat.Diffuse = MDiffuse;
    mat.Specular = MSpecular;
    
    tLight LightFinal = ComputeDirectionalLight(mat, SceneLighting, input.Normal, input.WorldPosition, LCameraPos);
    float4 Pixel = LightFinal.Ambient + LightFinal.Diffuse + LightFinal.Specular;
    
    return Pixel;
}