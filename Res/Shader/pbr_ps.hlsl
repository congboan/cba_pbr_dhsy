struct PSInput
{
    float4 position: SV_POSITION;
    float4 texcoord: TEXCOORD0;
    float4 normal: NORMAL;
    float4 worldPos: TEXCOORD1;
};
cbuffer xx:register(b3){
    float4 CameraPosition;
    float4 Reserved[1023];
}

static const float PI=3.141592;

float3 F(float3 inF0,float inHdotV)
{
    return inF0+(float3(1.0f,1.0f,1.0f)-inF0)*pow(1-inHdotV,5.0f);
}

float NDF(float3 inN,float3 inH,float3 inRoughness){
    float NdotH=max(0.0f,dot(inN,inH));
    float r4=pow(inRoughness,4.0f);
    float A=pow(NdotH,2.0f);
    float B=r4-1.0f;
    float C=A*B+1.0f;
    float D=pow(C,2.0f);
    float E=PI*D;

    return r4/E;
}

float Geometry(float inNdotX,float inRoughness)
{
    float k=pow(inRoughness+1.0f,2.0f)/8.0f;
    return inNdotX/(inNdotX*(1-k)+k);
}

float4 main(PSInput input): SV_TARGET
{
float3 finalColor=float3(0.0f,0.0f,0.0f);
float3 L=normalize(float3(1.0f,1.0f,-1.0f));
float3 N=normalize(input.normal.xyz);
float3 V=normalize(CameraPosition.xyz-input.worldPos.xyz);
float3 H=normalize(L+V);
float3 f0=float3(0.04,0.04,0.04);
float roughness=0.3f;

float NdotL=max(0.0f,dot(N,L));
float HdotV=max(0.0f,dot(H,V));
float NdotV=max(0.0f,dot(N,V));

//diffuse+specular
{
    if(NdotL>0.0f)
    {
        float3 Ks=F(f0,HdotV);
        float3 Kd=1-Ks;
        float D=NDF(N,H,roughness);
        float GL=Geometry(NdotL,roughness);
        float GV=Geometry(NdotV,roughness);
        float G=GL*GV;

        float3 diffuseColor=float3(1.0f,1.0f,1.0f);
        finalColor+=diffuseColor*NdotL;
        float3 specularColor=(Ks*D*G)/(4*NdotL*NdotV+0.0001);
        finalColor+=specularColor;
    }
}
//ambient color
{
    float3 ambientColor=float3(0.0f,0.0f,0.0f);
    finalColor+=ambientColor;
}


    return float4(finalColor,1.0f);
}