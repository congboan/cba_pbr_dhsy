#pragma once
#include "utils.h"
#include "BattleFireDirect3D12.h"
class FrameBuffer {
public:
	ID3D12DescriptorHeap* mRTVDescriptorHeap,*mDSVDescriptorHeap,* mCSRVDescriptorHeap, * mDSRVDescriptorHeap;
	ID3D12Resource* mColorBuffer,*mDSBuffer;
	DXGI_FORMAT mFormat;
	int mWidth, mHeight;
	FrameBuffer();
	void SetSize(uint32_t width, uint32_t height);
	void AttachColorBuffer(DXGI_FORMAT inFormat = DXGI_FORMAT_R32G32B32A32_FLOAT);
	void AttachDepthBuffer();
	void Finish();
	void SetClearColor(int inIndex, float inR, float inG, float inB, float inA);
	void SetClearDepthStencil(float depth, uint32_t stencil);
	FrameBufferRT* BeginRendering(ID3D12GraphicsCommandList *inCommandList=nullptr);
	void EndRendering(ID3D12GraphicsCommandList* inCommandList = nullptr);
};
