struct VertexData{
    float4 position: POSITION;
    float4 texcoord: TEXCOORD0;
    float4 normal: NORMAL;
};

struct VertexOutput{
    float4 position: SV_POSITION;
    float4 texcoord: TEXCOORD0;
    float4 normal: NORMAL;
    float4 worldPos: TEXCOORD1;
};

cbuffer PerObject:register(b0){
    float4x4 ProjectionMatrix;
    float4x4 ViewMatrix;
    float4x4 ModelMatrix;
    float4x4 ITModelMatrix;
    float4x4 Reserverd[1024];
};

VertexOutput main(VertexData inVertexData){ 
    VertexOutput o;
    float4 positionWS=mul(ModelMatrix,float4(inVertexData.position.xyz,1.0f));
    float4 positionVS=mul(ViewMatrix,positionWS);
    float4 positionCS=mul(ProjectionMatrix,positionVS);
    float4 normalWS=mul(ITModelMatrix,float4(inVertexData.normal.xyz,0.0f));
    o.position=positionCS;
    o.texcoord=inVertexData.texcoord;
    o.normal=normalWS;
    o.worldPos=positionWS;
    return o;
}