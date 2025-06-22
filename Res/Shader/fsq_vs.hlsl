struct VertexData{
    float4 position: POSITION;
    float4 texcoord: TEXCOORD0;
    float4 normal: NORMAL;
};

struct VertexOutput{
    float4 position: SV_POSITION;
    float4 texcoord: TEXCOORD0;
    float4 normal: NORMAL;
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
    float4 positionCS=float4(inVertexData.position.xyz,1.0f);
    o.position=positionCS;
    o.texcoord=inVertexData.texcoord;
    return o;
}