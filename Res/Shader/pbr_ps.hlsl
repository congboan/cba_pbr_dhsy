struct PSInput
{
    float4 position: SV_POSITION;
    float4 texcoord: TEXCOORD0;
    float4 normal: NORMAL;
    float4 worldPos: TEXCOORD1;
};

float4 main(PSInput input): SV_TARGET
{
float3 finalColor=float3(0.0f,0.0f,0.0f);
float3 ambient=float3(0.1f,0.0f,0.0f);
float3 diffuseColor=float3(0.0f,0.4f,0.0f);
float3 specularColor=float3(0.0f,0.0f,0.6f);
finalColor=ambient+diffuseColor+specularColor;
    return input.texcoord;
}