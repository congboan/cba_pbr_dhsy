#pragma once
#include "BattleFireDirect3D12.h"
#include "Material.h"
struct StaticMeshVertexData {
	DirectX::XMFLOAT4 mPosition;
	DirectX::XMFLOAT4 mTexcoord;
	DirectX::XMFLOAT4 mNormal;
};
struct StaticMeshVertexDataEx : public StaticMeshVertexData {
	DirectX::XMFLOAT4 mTangent;
};
struct D3D12SubMesh : public SubMesh {
	ID3D12Resource * mIBO;
	D3D12_INDEX_BUFFER_VIEW mIBOView;
	D3D12SubMesh() {
		mIBO = nullptr;
	}
};
class StaticMeshComponent {
public:
	StaticMeshVertexData* mVertexData;
	int mVertexCount;
	int mVertexDataSize;
	ID3D12Resource* mVBO;
	D3D12_VERTEX_BUFFER_VIEW mVBOView;
	std::unordered_map<std::string, D3D12SubMesh*> mSubMeshes;
	Material* mMaterial;
	void SetPosition(int index, float x, float y, float z, float w = 1.0f);
	void SetTexcoord(int index, float x, float y, float z = 1.0f, float w = 1.0f);
	void SetNormal(int index, float x, float y, float z, float w = 0.0f);
	void InitFromFile(ID3D12GraphicsCommandList* inCmdList, const char* inFilePath);
	void InitFromFile2(ID3D12GraphicsCommandList* inCmdList, const char* inFilePath);
	StaticMeshComponent();
	void Render(ID3D12GraphicsCommandList* inCmdList);
};
class FullScreenQuadMeshComponent : public StaticMeshComponent {
public:
	void Init(ID3D12GraphicsCommandList* inCommandList);
};
struct SkinedMeshVertexData {
	DirectX::XMFLOAT4 mPosition;
	DirectX::XMFLOAT4 mTexcoord;
	DirectX::XMFLOAT4 mNormal;
	DirectX::XMFLOAT4 mBone;
	DirectX::XMFLOAT4 mWeights;
};
struct BaseConstantBufferData {
	DirectX::XMFLOAT4X4 mProjectionMatrix;
	DirectX::XMFLOAT4X4 mViewMatrix;
	DirectX::XMFLOAT4X4 mTranslateMatrix;
	DirectX::XMFLOAT4X4 mRotateMatrix;
	DirectX::XMFLOAT4X4 mScaleMatrix;
};
struct SkinedMeshConstantBufferData : public BaseConstantBufferData {
	DirectX::XMFLOAT4X4 mTPoseInverseMatrix[128];
	DirectX::XMFLOAT4X4 mBoneMatrix[128];
};
struct BaseVec4ConstantBufferData {
	DirectX::XMFLOAT4 mCameraPosition;
	DirectX::XMFLOAT4 mReserved[4095];
};
struct SkinedMeshSubMesh {
	SkinedMeshVertexData* mVertexData;
	unsigned int* mIndexes;
	int mVertexCount;
	int mIndexCount;
	ID3D12Resource* mVBO, * mIBO;
	D3D12_VERTEX_BUFFER_VIEW mVBOView;
	D3D12_INDEX_BUFFER_VIEW mIBOView;
	PipelineStateObject* mPipelineStateObject;
	ID3D12DescriptorHeap* mTextureDescriptorHeap;
	ID3D12Resource* mTexture;
	SkinedMeshSubMesh();
	void Render(ID3D12GraphicsCommandList* inCmdList);
};
struct AnimationFrame {
	DirectX::XMFLOAT4X4 mBones[128];
};
class SkinedMeshComponent {
public:
	std::vector<SkinedMeshSubMesh*> mSubMeshes;
	int mFrameRate;
	int mFrameCount;
	int mMeshCount;
	int mBoneCount;
	DirectX::XMFLOAT4X4 mTPoseInverse[128];
	std::vector<AnimationFrame*> mAnimationFrames;
	AnimationFrame* mCurrentAnimationFrame;
	float mAnimationTime;
	bool mPlayAnimation;
	void InitFromFile(ID3D12GraphicsCommandList* inCmdList,const char* inFilePath);
	void Update(float inDeltaTime);
};
DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);
WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);
int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);
int LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow);
