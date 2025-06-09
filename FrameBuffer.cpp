#include "FrameBuffer.h"

FrameBuffer::FrameBuffer() {
	mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	mRTVDescriptorHeap = CreateDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDSVDescriptorHeap = CreateDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCSRVDescriptorHeap = CreateDescriptorHeap(1);
	mDSRVDescriptorHeap = CreateDescriptorHeap(1);
}
void FrameBuffer::SetSize(uint32_t width, uint32_t height) {
	mWidth = width;
	mHeight = height;
}
void FrameBuffer::AttachColorBuffer(DXGI_FORMAT inFormat /* = DXGI_FORMAT_R32G32B32A32_FLOAT */) {
	mFormat = inFormat;

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
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
		IID_PPV_ARGS(&mColorBuffer));

	//RTV
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = mFormat;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

	GetDirect3DDevice()->CreateRenderTargetView(mColorBuffer, &rtvDesc, mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	
	//SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	GetDirect3DDevice()->CreateShaderResourceView(mColorBuffer, &srvDesc, mCSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}
void FrameBuffer::AttachDepthBuffer() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES textureHeapProperties = InitD3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	GetDirect3DDevice()->CreateCommittedResource(
		&textureHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mDSBuffer));

	//DS SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	GetDirect3DDevice()->CreateShaderResourceView(mDSBuffer, &srvDesc, mDSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	//DS RTV
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	GetDirect3DDevice()->CreateDepthStencilView(mDSBuffer, &dsvDesc, mDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}
void FrameBuffer::Finish() {

}
void FrameBuffer::SetClearColor(int inIndex, float inR, float inG, float inB, float inA) {

}
void FrameBuffer::SetClearDepthStencil(float depth, uint32_t stencil) {

}
FrameBufferRT* FrameBuffer::BeginRendering(ID3D12GraphicsCommandList* inCommandList/* =nullptr */) {
	D3D12_RESOURCE_BARRIER berrierCRT = InitResourceBarrier(mColorBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	inCommandList->ResourceBarrier(1, &berrierCRT);
	D3D12_RESOURCE_BARRIER berrierDSRT = InitResourceBarrier(mDSBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	inCommandList->ResourceBarrier(1, &berrierDSRT);
	FrameBufferRT* frameBufferRT = new FrameBufferRT;
	frameBufferRT->mColorBuffer.mDescriptorHeap = mRTVDescriptorHeap;
	frameBufferRT->mColorBuffer.mFormat = mFormat;
	frameBufferRT->mColorBuffer.mResource = mColorBuffer;
	frameBufferRT->mColorBuffer.mRTV = mRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	frameBufferRT->mDSBuffer.mDescriptorHeap = mDSVDescriptorHeap;
	frameBufferRT->mDSBuffer.mFormat = DXGI_FORMAT_R24G8_TYPELESS;
	frameBufferRT->mDSBuffer.mResource = mDSBuffer;
	frameBufferRT->mDSBuffer.mRTV = mDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	inCommandList->OMSetRenderTargets(1, &frameBufferRT->mColorBuffer.mRTV, FALSE, &frameBufferRT->mDSBuffer.mRTV);
	D3D12_VIEWPORT viewport = { 0.0f,0.0f,float(mWidth),float(mHeight),0.0f,1.0f };
	D3D12_RECT scissorRect = { 0,0,mWidth,mHeight };
	inCommandList->RSSetViewports(1, &viewport);
	inCommandList->RSSetScissorRects(1, &scissorRect);
	const float hdrClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	inCommandList->ClearRenderTargetView(frameBufferRT->mColorBuffer.mRTV, hdrClearColor, 0, nullptr);
	inCommandList->ClearDepthStencilView(frameBufferRT->mDSBuffer.mRTV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	return frameBufferRT;
}
void FrameBuffer::EndRendering(ID3D12GraphicsCommandList* inCommandList /* = nullptr */) {
	D3D12_RESOURCE_BARRIER berrierCRT = InitResourceBarrier(mColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	inCommandList->ResourceBarrier(1, &berrierCRT);
	D3D12_RESOURCE_BARRIER berrierDSRT = InitResourceBarrier(mDSBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
	inCommandList->ResourceBarrier(1, &berrierDSRT);
}