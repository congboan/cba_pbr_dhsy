#include "BoxCapture.h"

BoxCapture::BoxCapture() {
}
void BoxCapture::Init(int inResolution /* = 512 */, DXGI_FORMAT inFormat /* = DXGI_FORMAT_R32G32B32A32_FLOAT */, int inMipmapLevelNum /* = 1 */) {
	mSize = inResolution;
	mFormat = inFormat;
	mMipmapLevelNum = inMipmapLevelNum;
	mRTVDescriptorHeap = CreateDescriptorHeap(inMipmapLevelNum * 6, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDRTVDescriptorHeap = CreateDescriptorHeap(inMipmapLevelNum * 6, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCSRVDescriptorHeap = CreateDescriptorHeap(1);

	{
		D3D12_RESOURCE_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Alignment = 0;
		texDesc.Width = mSize;
		texDesc.Height = mSize;
		texDesc.DepthOrArraySize = 6;
		texDesc.MipLevels = inMipmapLevelNum;
		texDesc.Format = mFormat;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = mFormat;
		optClear.Color[0] = 0.0f;
		optClear.Color[1] = 0.0f;
		optClear.Color[2] = 0.0f;
		optClear.Color[3] = 0.0f;

		D3D12_HEAP_PROPERTIES textureHeapProperties = InitD3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		GetDirect3DDevice()->CreateCommittedResource(
			&textureHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&optClear,
			IID_PPV_ARGS(&mCubeMapBuffer));

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = mFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = inMipmapLevelNum;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		GetDirect3DDevice()->CreateShaderResourceView(mCubeMapBuffer, &srvDesc, mCSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		UINT rtvDescriptorSize = GetDirect3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		for (int i = 0; i < 6; ++i)
		{
			for (int mipmapLevel = 0; mipmapLevel < inMipmapLevelNum; mipmapLevel++)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE temp = cpuDescriptorHandle;
				temp.ptr += (i * inMipmapLevelNum * rtvDescriptorSize + mipmapLevel * rtvDescriptorSize);
				mRTV[i][mipmapLevel] = temp;
				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				rtvDesc.Format = mFormat;
				rtvDesc.Texture2DArray.MipSlice = mipmapLevel;
				rtvDesc.Texture2DArray.PlaneSlice = 0;

				rtvDesc.Texture2DArray.FirstArraySlice = i;

				rtvDesc.Texture2DArray.ArraySize = 1;

				GetDirect3DDevice()->CreateRenderTargetView(mCubeMapBuffer, &rtvDesc, mRTV[i][mipmapLevel]);
			}
		}
	}
	{
		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = mSize;
		depthStencilDesc.Height = mSize;
		depthStencilDesc.DepthOrArraySize = 6;
		depthStencilDesc.MipLevels = inMipmapLevelNum;
		depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;

		D3D12_HEAP_PROPERTIES textureHeapProperties = InitD3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		GetDirect3DDevice()->CreateCommittedResource(
			&textureHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optClear,
			IID_PPV_ARGS(&mCubeMapDSBuffer));

		D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = mDRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		UINT dsvDescriptorSize = GetDirect3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		for (int i = 0; i < 6; ++i)
		{
			for (int mipmapLevel = 0; mipmapLevel < inMipmapLevelNum; mipmapLevel++)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE temp = cpuDescriptorHandle;
				temp.ptr += (i * inMipmapLevelNum * dsvDescriptorSize + mipmapLevel * dsvDescriptorSize);
				mDRTV[i][mipmapLevel] = temp;
				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				dsvDesc.Texture2DArray.MipSlice = mipmapLevel;
				dsvDesc.Texture2DArray.FirstArraySlice = i;
				dsvDesc.Texture2DArray.ArraySize = 1;

				GetDirect3DDevice()->CreateDepthStencilView(mCubeMapDSBuffer, &dsvDesc, mDRTV[i][mipmapLevel]);
			}
		}
	}
}