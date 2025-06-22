#include "Scene.h"
#include "BattleFireDirect3D12.h"
#include "FrameBuffer.h"
#include "Node.h"
#include "Camera.h"
#include "Material.h"

FrameBuffer* g_HDRFBO = nullptr;
Node *g_node = nullptr, *g_fsqNode = nullptr;
DirectX::XMMATRIX g_projectionMatrix;
Camera g_mainCamera;

void InitSphere(ID3D12GraphicsCommandList* inCommandList)
{
    g_node = new Node;
    StaticMeshComponent* staticMeshComponent = new StaticMeshComponent;
    staticMeshComponent->InitFromFile(inCommandList, "Res/Model/Sphere.staticmesh");
    Material* material = new Material(L"Res/Shader/pbr_vs.hlsl", L"Res/Shader/pbr_ps.hlsl");
    material->SetCullMode(D3D12_CULL_MODE_FRONT);
    staticMeshComponent->mMaterial = material;
    g_node->mStaticMeshComponent = staticMeshComponent;
}

void InitToneMapping(ID3D12GraphicsCommandList* inCommandList)
{
    g_fsqNode = new Node;
    FullScreenQuadMeshComponent* fsq = new FullScreenQuadMeshComponent;
    fsq->Init(inCommandList);
    g_fsqNode->mStaticMeshComponent = fsq;
    Material* material = new Material(L"Res/Shader/fsq_vs.hlsl", L"Res/Shader/fsq_ps.hlsl");
    material->SetPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    material->SetTexture(0, DXGI_FORMAT_R32G32B32A32_FLOAT, g_HDRFBO->mColorBuffer);
    g_fsqNode->mStaticMeshComponent->mMaterial = material;
}

void InitScene(int inWidth, int inHeight)
{
    g_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH((45.0f * 3.14f) / 180.0f,
                                                           float(inWidth) / float(inHeight), 0.1f, 1000.0f);
    g_mainCamera.Init(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), 5.0f,
                      DirectX::XMVectorSet(0.0f, -0.2f, 1.0f, 0.0f));
    g_HDRFBO = new FrameBuffer;
    g_HDRFBO->SetSize(inWidth, inHeight);
    g_HDRFBO->AttachColorBuffer(DXGI_FORMAT_R32G32B32A32_FLOAT);
    g_HDRFBO->AttachDepthBuffer();
    RHICommandList rhiCommandList;
    InitSphere(rhiCommandList.mCommandList);
    InitToneMapping(rhiCommandList.mCommandList);
}

void RenderOneFrame(float inDeltaTime)
{
    RHICommandList rhiCommandList;

    FrameBufferRT* rt = g_HDRFBO->BeginRendering(rhiCommandList.mCommandList);

    DirectX::XMFLOAT4 temp;
    DirectX::XMStoreFloat4(&temp,g_mainCamera.mPosition);
    g_node->mStaticMeshComponent->mMaterial->SetCameraWorldPosition(temp.x, temp.y, temp.z);
    g_node->Draw(rhiCommandList.mCommandList, g_projectionMatrix, g_mainCamera, rt->mColorBuffer.mFormat,
                 rt->mDSBuffer.mFormat);
    g_HDRFBO->EndRendering(rhiCommandList.mCommandList);
    delete rt;
    rt = BeginRenderFrame(rhiCommandList.mCommandList);

    g_fsqNode->Draw(rhiCommandList.mCommandList, g_projectionMatrix, g_mainCamera, rt->mColorBuffer.mFormat,
                    rt->mDSBuffer.mFormat);

    EndRenderFrame(rhiCommandList.mCommandList);
    delete rt;
}
