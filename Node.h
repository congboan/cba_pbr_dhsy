#pragma once
#include "Utils.h"
#include "Mesh.h"
#include "BattleFireDirect3D12.h"
class Camera;
class Node {
public:
	StaticMeshComponent* mStaticMeshComponent;
	SkinedMeshComponent* mSkinedMeshComponent;
	Mat4UniformBufferData* mMat4UniformBufferData;
	DirectX::XMMATRIX mModelMatrix;
	Node();
	void UpdateConstantBufferData(DirectX::XMMATRIX&inProjectionMatrix, DirectX::XMMATRIX& inViewMatrix);
	void UpdateModelMatrix();
	void SetPosition(float inX, float inY, float inZ);
	void Draw(ID3D12GraphicsCommandList* inCommandList, DirectX::XMMATRIX& inProjectionMatrix, Camera& inCamera, DXGI_FORMAT inColorRTFormat, DXGI_FORMAT inDSRTFormat);
};