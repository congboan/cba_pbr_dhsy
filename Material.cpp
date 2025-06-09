#include "Material.h"
#include "Mesh.h"

ID3D12RootSignature* Material::mRootSignature = nullptr;
Material::Material(LPCWSTR inVSPath, LPCWSTR inFSPath) {
	mDescriptorHeap = CreateDescriptorHeap(28);
	mbNeedUpdatePSO = true;
	mVec4s = new Vec4UniformBufferData(1024);
	mGPUProgram = GPUProgram::GetGPUProgram(inVSPath, inFSPath);
	mD3D12_PRIMITIVE_TOPOLOGY = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mD3D12_PRIMITIVE_TOPOLOGY_TYPE = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	memset(mTextures, 0, sizeof(mTextures));
	mbEnableDepthTest = true;
	mbEnableWriteDepth = true;
	mPipelineState = nullptr;
	mFrontCounterClockwise = true;
	mD3D12_CULL_MODE = D3D12_CULL_MODE_BACK;
}
void Material::EnableDepthTest(bool inEnable) {
	mbEnableDepthTest = inEnable;
	mbEnableWriteDepth = inEnable;
	mbNeedUpdatePSO = true;
}
void Material::Active(ID3D12GraphicsCommandList* inCommandList, int inVertexDataSize, DXGI_FORMAT inColorRTFormat, DXGI_FORMAT inDSRTFormat) {
	if (mbNeedUpdatePSO) {
		if (mPipelineState != nullptr) {
			mPipelineState->Release();
			mPipelineState = nullptr;
		}
		if (inVertexDataSize == sizeof(StaticMeshVertexData)) {
			mPipelineState = GenPipelineStateObject(mRootSignature, mGPUProgram, mD3D12_PRIMITIVE_TOPOLOGY_TYPE,
				mbEnableDepthTest, mbEnableWriteDepth, inColorRTFormat, mD3D12_CULL_MODE, mFrontCounterClockwise);
		}
		else {
			mPipelineState = GenPipelineStateObject2(mRootSignature, mGPUProgram, mD3D12_PRIMITIVE_TOPOLOGY_TYPE,
				mbEnableDepthTest, mbEnableWriteDepth, inColorRTFormat, mD3D12_CULL_MODE, mFrontCounterClockwise);
		}
		mbNeedUpdatePSO = false;
	}
	if (mPipelineState != nullptr) {
		mVec4s->UpdateGPUData();
		inCommandList->IASetPrimitiveTopology(mD3D12_PRIMITIVE_TOPOLOGY);
		inCommandList->SetPipelineState(mPipelineState);
		inCommandList->SetGraphicsRootSignature(mRootSignature);
		ID3D12DescriptorHeap* descriptorHeaps[] = { mDescriptorHeap };
		inCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
		inCommandList->SetGraphicsRootConstantBufferView(3, mVec4s->mResource->GetGPUVirtualAddress());
		inCommandList->SetGraphicsRootDescriptorTable(4, mDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}
}
void Material::SetVec4(int inIndex, float* inVec4) {
	mVec4s->SetVec4(inIndex, inVec4);
}
void Material::SetVec4(int inIndex, float x, float y, float z, float w) {
	float v[4];
	v[0] = x;
	v[1] = y;
	v[2] = z;
	v[3] = w;
	SetVec4(inIndex, v);
}
void Material::SetCameraWorldPosition(float inX, float inY, float inZ, float inW /* = 1.0f */) {
	SetVec4(0, inX, inY, inZ, inW);
}
void Material::SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY inD3D12_PRIMITIVE_TOPOLOGY) {
	mD3D12_PRIMITIVE_TOPOLOGY = inD3D12_PRIMITIVE_TOPOLOGY;
	if (inD3D12_PRIMITIVE_TOPOLOGY == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST ||
		inD3D12_PRIMITIVE_TOPOLOGY == D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP) {
		mD3D12_PRIMITIVE_TOPOLOGY_TYPE = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}
	else {
		//
	}
	mbNeedUpdatePSO = true;
}
void Material::SetFrontCounterClockwise(bool inFrontCounterClockwise /* = true */) {
	mFrontCounterClockwise = inFrontCounterClockwise;
	mbNeedUpdatePSO = true;
}
void Material::SetCullMode(D3D12_CULL_MODE inD3D12_CULL_MODE) {
	mD3D12_CULL_MODE = inD3D12_CULL_MODE;
	mbNeedUpdatePSO = true;
}
void Material::SetTexture(int inIndex, DXGI_FORMAT inFormat, ID3D12Resource* inTexture, int inMipmapLevelNum) {
	mTextures[inIndex] = inTexture;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = inFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = inMipmapLevelNum;

	UINT descriptorHandleSize = GetDirect3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	srvHandle.ptr += inIndex * descriptorHandleSize;
	GetDirect3DDevice()->CreateShaderResourceView(inTexture, &srvDesc, srvHandle);
}
void Material::SetTextureCube(int inIndex, DXGI_FORMAT inFormat, ID3D12Resource* inTexture, int inMipmapLevelNum) {
	mTextures[inIndex] = inTexture;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = inFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = inMipmapLevelNum;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	UINT descriptorHandleSize = GetDirect3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	srvHandle.ptr += inIndex * descriptorHandleSize;
	GetDirect3DDevice()->CreateShaderResourceView(inTexture, &srvDesc, srvHandle);
}