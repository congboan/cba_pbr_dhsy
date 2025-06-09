#pragma once

#include <windows.h>
#include <wincodec.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
class GPUProgram {
public:
	ID3DBlob* mVertexShaderData;
	ID3DBlob* mFragmentShaderData;
	D3D12_SHADER_BYTECODE mVertexShader;
	D3D12_SHADER_BYTECODE mFragmentShader;
	GPUProgram();
	~GPUProgram();
	static std::unordered_map<std::wstring, GPUProgram*> sCachedGPUProgram;
	static GPUProgram* GetGPUProgram(LPCWSTR inVSPath, LPCWSTR inFSPath);
	static void CleanUp();
};
struct ConstantBuffer {
	ID3D12DescriptorHeap* mID3D12DescriptorHeap;
	ID3D12Resource* mID3D12Resource;
};
struct Texture {
	ID3D12DescriptorHeap* mDescriptorHeap;
	ID3D12Resource* mResource;
	D3D12_CPU_DESCRIPTOR_HANDLE mRTV;
	DXGI_FORMAT mFormat;
};
struct FrameBufferRT {
	Texture mColorBuffer;
	Texture mDSBuffer;
};
struct PipelineStateObject {
	ID3D12PipelineState* mPSO;
	ID3D12DescriptorHeap * mDescriptorHeap;
	PipelineStateObject();
};
bool InitializeDirect3D12(HWND inHWND, int inWidth, int inHeight);
ID3D12Device* GetDirect3DDevice();
void WaitForPreviousFrame();
FrameBufferRT* BeginRenderFrame(ID3D12GraphicsCommandList* inCmdList);
void EndRenderFrame(ID3D12GraphicsCommandList* inCmdList);
void Direct3DSwapBuffers();
void CleanUpDirect3D12();
ID3D12GraphicsCommandList* GetCommandList(ID3D12PipelineState* inPSO=nullptr);
void EndCommandList(int inDeltaFenceValue = 0);
struct RHICommandList {
	ID3D12GraphicsCommandList* mCommandList;
	RHICommandList();
	~RHICommandList();
	ID3D12GraphicsCommandList* operator->() {
		return mCommandList;
	}
};
ID3D12Resource* GenBufferObject(ID3D12GraphicsCommandList* inCommandList, void* inData, int inDataLen, D3D12_RESOURCE_STATES inFinalResourceState);
ID3D12DescriptorHeap* CreateDescriptorHeap(int inDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE inD3D12_DESCRIPTOR_HEAP_TYPE = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
ID3D12Resource* GenConstantBuffer(int inDataLen = 65536);
void UpdateConstantBuffer(ID3D12Resource* inGPUConstantBuffer, void* inData, int inDataLen);
ID3D12PipelineState* GenPipelineStateObject(ID3D12RootSignature* inRootSignature,GPUProgram*inGPUProgram,D3D12_PRIMITIVE_TOPOLOGY_TYPE inD3D12_PRIMITIVE_TOPOLOGY_TYPE,
	bool inEnableDepthTest = true, bool inEnableDepthWrite = true, DXGI_FORMAT inColorRTFormat = DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_CULL_MODE inCullMode = D3D12_CULL_MODE_BACK, bool inFrontCounterClockwise = true);
ID3D12PipelineState* GenPipelineStateObject2(ID3D12RootSignature* inRootSignature, GPUProgram* inGPUProgram, D3D12_PRIMITIVE_TOPOLOGY_TYPE inD3D12_PRIMITIVE_TOPOLOGY_TYPE,
	bool inEnableDepthTest = true, bool inEnableDepthWrite = true, DXGI_FORMAT inColorRTFormat = DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_CULL_MODE inCullMode = D3D12_CULL_MODE_BACK, bool inFrontCounterClockwise = true);
ID3D12Resource* CreateTexture2D(ID3D12GraphicsCommandList* inCommandList, LPCTSTR inFilePath, DXGI_FORMAT& inFormat);
ID3D12Resource* CreateTextureCube(ID3D12GraphicsCommandList* inCommandList, LPCTSTR* inFilePath);
ID3D12Resource* LoadHDRITextureFromFile(ID3D12GraphicsCommandList* inCommandList, const char* inFilePath, DXGI_FORMAT inFormat);
D3D12_HEAP_PROPERTIES InitD3D12HeapProperties(D3D12_HEAP_TYPE inD3D12_HEAP_TYPE);
D3D12_BLEND_DESC InitDefaultBlendDesc();
D3D12_RESOURCE_BARRIER InitResourceBarrier(ID3D12Resource* pResource,D3D12_RESOURCE_STATES stateBefore,D3D12_RESOURCE_STATES stateAfter);