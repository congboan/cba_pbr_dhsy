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
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }
extern "C" {
#include "stbi/stb_image.h"
}
struct SubMesh {
	unsigned int* mIndexes;
	int mIndexCount;
};
struct Mat4UniformBufferData {
	int mMat4Count;
	bool mbNeedSyncData;
	ID3D12Resource* mResource;
	float mData[16384];
	Mat4UniformBufferData(int inMat4Count);
	void SetMat4(int inIndex, void* inMat4);
	void UpdateGPUData();
};
struct Vec4UniformBufferData {
	int mVec4Count;
	bool mbNeedSyncData;
	float mData[4096];
	ID3D12Resource* mResource;
	Vec4UniformBufferData(int inVec4Count);
	void SetVec4(int inIndex, float* inVec4);
	void UpdateGPUData();
};
struct ImageData {
	DXGI_FORMAT mDXGI_FORMAT;
	int mImageWidth,mImageHeight;
	int mBPP;
	int mBPR;
	BYTE* mPixelData;
	int mPixelDataLen;
};
DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);
WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);
int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);
ImageData* LoadImageDataFromFile(LPCWSTR filename);