#pragma once
#include "utils.h"
#include "BattleFireDirect3D12.h"
class Material {
public:
	GPUProgram* mGPUProgram;
	ID3D12PipelineState* mPipelineState;
	ID3D12DescriptorHeap* mDescriptorHeap;
	Vec4UniformBufferData* mVec4s;
	ID3D12Resource* mTextures[28];
	bool mbNeedUpdatePSO, mbEnableDepthTest, mbEnableWriteDepth;
	D3D12_PRIMITIVE_TOPOLOGY mD3D12_PRIMITIVE_TOPOLOGY;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE mD3D12_PRIMITIVE_TOPOLOGY_TYPE;
	D3D12_CULL_MODE mD3D12_CULL_MODE;
	bool mFrontCounterClockwise;
	static ID3D12RootSignature* mRootSignature;
	Material(LPCWSTR inVSPath, LPCWSTR inFSPath);
	void EnableDepthTest(bool inEnable);
	void SetVec4(int inIndex, float* v);
	void SetVec4(int inIndex, float x, float y, float z, float w = 0.0f);
	void SetCameraWorldPosition(float inX, float inY, float inZ, float inW = 1.0f);
	void SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY inD3D12_PRIMITIVE_TOPOLOGY);
	void SetFrontCounterClockwise(bool inFrontCounterClockwise = true);
	void SetCullMode(D3D12_CULL_MODE inD3D12_CULL_MODE);
	void SetTexture(int inIndex, DXGI_FORMAT inFormat, ID3D12Resource* inTexture, int inMipmapLevelNum = 1);
	void SetTextureCube(int inIndex, DXGI_FORMAT inFormat, ID3D12Resource* inTexture, int inMipmapLevelNum = 1);
	void Active(ID3D12GraphicsCommandList* inCommandList,int inVertexDataSize, DXGI_FORMAT inColorRTFormat, DXGI_FORMAT inDSRTFormat);
};
