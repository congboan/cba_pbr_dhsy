#pragma once
#include "utils.h"
#include "BattleFireDirect3D12.h"
class BoxCapture {
public:
	ID3D12DescriptorHeap * mRTVDescriptorHeap, * mCSRVDescriptorHeap, * mDRTVDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE mRTV[6][5],mDRTV[6][5];
	ID3D12Resource* mCubeMapBuffer,*mCubeMapDSBuffer;
	DXGI_FORMAT mFormat;
	int mSize;
	int mMipmapLevelNum;
	BoxCapture();
	void Init(int inResolution = 512, DXGI_FORMAT inFormat = DXGI_FORMAT_R32G32B32A32_FLOAT, int inMipmapLevelNum = 1);
};
