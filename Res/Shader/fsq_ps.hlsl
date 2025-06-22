Texture2D t0:register(t0);
SamplerState samplerStateLC:register(s0);//liner clamp

struct PSInput
{
    float4 position: SV_POSITION;
    float4 texcoord: TEXCOORD0;
};

static const float gamma=2.2f;
float3 GammaEncode(float3 inHDRColor)
{
    return pow(inHDRColor,float3(1.0f/gamma,1.0f/gamma,1.0f/gamma));
}

float4 main(PSInput input): SV_TARGET
{
    float3 hdrColor=t0.Sample(samplerStateLC,input.texcoord.xy).rgb;
    float3 mappedColor=hdrColor/(hdrColor+float3(1.0f,1.0f,1.0f));
    return float4(GammaEncode(mappedColor),1.0f);
}