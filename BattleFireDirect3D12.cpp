#include "BattleFireDirect3D12.h"
#include "Utils.h"
#include "Material.h"
static ID3D12Device* sD3DDevice = nullptr;
static int sCurrentFrameIndex=-1;
const int sDefaultFrameBufferCount = 2;
static int sViewportWidth=0, sViewportHeight=0;
static ID3D12Resource* sRenderTargets[sDefaultFrameBufferCount];
static DXGI_FORMAT sColorRTFormat = DXGI_FORMAT_R8G8B8A8_UNORM, sDSRTFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
static IDXGISwapChain3* sSwapChain = nullptr;
static ID3D12CommandQueue* sCommandQueue = nullptr;
static int sRTVDescriptorSize = 0;
static ID3D12DescriptorHeap* sRTVDescriptorHeap = nullptr;
static ID3D12DescriptorHeap* sDSDescriptorHeap;
static ID3D12Resource* sDepthStencilBuffer;
static ID3D12CommandAllocator* sCommandAllocator = nullptr;
static ID3D12GraphicsCommandList* sCommandList = nullptr;
static ID3D12Fence* sFence;
static HANDLE sFenceEvent;
static UINT64 sFenceValue;
static UINT gRTV_DescriptorSize = 0;
static UINT gDSV_DescriptorSize = 0;
static UINT gCBV_SRV_UAV_DescriptorSize = 0;
static UINT gSAMPLER_DescriptorSize = 0;

ID3D12RootSignature* InitializeRootSignature() {
	D3D12_DESCRIPTOR_RANGE  fsSRVDescriptorRanges[16];
	for (int i = 0; i < 16; i++) {
		fsSRVDescriptorRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		fsSRVDescriptorRanges[i].RegisterSpace = 0;
		fsSRVDescriptorRanges[i].BaseShaderRegister = i;
		fsSRVDescriptorRanges[i].NumDescriptors = 1;
		fsSRVDescriptorRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}
	D3D12_ROOT_DESCRIPTOR_TABLE fsSRVDescriptorTable;
	fsSRVDescriptorTable.NumDescriptorRanges = _countof(fsSRVDescriptorRanges);
	fsSRVDescriptorTable.pDescriptorRanges = fsSRVDescriptorRanges;

	D3D12_ROOT_PARAMETER  rootParameters[5];
	for (int i = 0; i < 3; i++) {
		rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[i].Descriptor.RegisterSpace = 0;
		rootParameters[i].Descriptor.ShaderRegister = i;
	}
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.RegisterSpace = 0;
	rootParameters[3].Descriptor.ShaderRegister = 3;

	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[4].DescriptorTable = fsSRVDescriptorTable;
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC linearSamplerDescs[2] = {};
	linearSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	linearSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	linearSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	linearSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	linearSamplerDescs[0].MipLODBias = 0;
	linearSamplerDescs[0].MaxAnisotropy = 0;
	linearSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	linearSamplerDescs[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	linearSamplerDescs[0].MinLOD = 0.0f;
	linearSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	linearSamplerDescs[0].RegisterSpace = 0;
	linearSamplerDescs[0].ShaderRegister = 0;
	linearSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	linearSamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	linearSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	linearSamplerDescs[1].MipLODBias = 0;
	linearSamplerDescs[1].MaxAnisotropy = 0;
	linearSamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	linearSamplerDescs[1].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	linearSamplerDescs[1].MinLOD = 0.0f;
	linearSamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	linearSamplerDescs[1].RegisterSpace = 0;
	linearSamplerDescs[1].ShaderRegister = 1;
	linearSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = _countof(linearSamplerDescs);
	rootSignatureDesc.pStaticSamplers = linearSamplerDescs;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* signature;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
	if (FAILED(hr))
	{
		return nullptr;
	}
	ID3D12RootSignature* d3d12RootSignature;
	hr = GetDirect3DDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&d3d12RootSignature));
	if (FAILED(hr))
	{
		return nullptr;
	}
	return d3d12RootSignature;
}
std::unordered_map<std::wstring, GPUProgram*> GPUProgram::sCachedGPUProgram;
GPUProgram::GPUProgram() {
	mVertexShaderData = nullptr;
	mFragmentShaderData = nullptr;
}
GPUProgram::~GPUProgram() {
	if (mVertexShaderData != nullptr) {
		mVertexShaderData->Release();
		mVertexShaderData = nullptr;
	}
	if (mFragmentShaderData != nullptr) {
		mFragmentShaderData->Release();
		mFragmentShaderData = nullptr;
	}
}
GPUProgram* GPUProgram::GetGPUProgram(LPCWSTR inVSPath, LPCWSTR inFSPath) {
	std::wstring gpuProgramName = inVSPath;
	gpuProgramName += inFSPath;
	auto iter = sCachedGPUProgram.find(gpuProgramName);
	if (iter != sCachedGPUProgram.end()) {
		return iter->second;
	}
	GPUProgram* gpuProgram = new GPUProgram();
	ID3DBlob* errorBuff;
	HRESULT hr = D3DCompileFromFile(inVSPath,
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&gpuProgram->mVertexShaderData,
		&errorBuff);
	if (FAILED(hr))
	{
		printf("D3DCompileFromFile vs_5_0 failed %s\n", (char*)errorBuff->GetBufferPointer());
		errorBuff->Release();
		delete gpuProgram;
		return nullptr;
	}
	gpuProgram->mVertexShader.BytecodeLength = gpuProgram->mVertexShaderData->GetBufferSize();
	gpuProgram->mVertexShader.pShaderBytecode = gpuProgram->mVertexShaderData->GetBufferPointer();

	hr = D3DCompileFromFile(inFSPath,
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&gpuProgram->mFragmentShaderData,
		&errorBuff);
	if (FAILED(hr))
	{
		printf("D3DCompileFromFile ps_5_0 failed %s\n", (char*)errorBuff->GetBufferPointer());
		errorBuff->Release();
		delete gpuProgram;
		return nullptr;
	}
	gpuProgram->mFragmentShader.BytecodeLength = gpuProgram->mFragmentShaderData->GetBufferSize();
	gpuProgram->mFragmentShader.pShaderBytecode = gpuProgram->mFragmentShaderData->GetBufferPointer();
	sCachedGPUProgram[gpuProgramName] = gpuProgram;
	return gpuProgram;
}
void GPUProgram::CleanUp() {
	for (auto iter = sCachedGPUProgram.begin(); iter != sCachedGPUProgram.end(); iter++) {
		delete iter->second;
	}
	sCachedGPUProgram.clear();
}
PipelineStateObject::PipelineStateObject() {
}
struct D3DHardwareMemoryInfo {
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT mLayout;
	UINT64 mRowSizedInBytes;
	UINT mRowCount;
};
ID3D12Resource* GenBufferObject(ID3D12GraphicsCommandList* inCommandList, void* inData, int inDataLen, D3D12_RESOURCE_STATES inFinalResourceState) {
	ID3D12Resource* bufferObject;
	D3D12_HEAP_PROPERTIES d3d12_heap_properties = InitD3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC d3d12ResourceDesc = {
		D3D12_RESOURCE_DIMENSION_BUFFER, 0, inDataLen, 1, 1, 1,
		DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE
	};
	GetDirect3DDevice()->CreateCommittedResource(
		&d3d12_heap_properties,
		D3D12_HEAP_FLAG_NONE,
		&d3d12ResourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&bufferObject));
	if (bufferObject == nullptr) {
		return nullptr;
	}
	D3D12_RESOURCE_DESC Desc = bufferObject->GetDesc();
	UINT64 memorySizeUsed = 0;
	D3DHardwareMemoryInfo hardwareMemoryInfo = { 0 };
	
	GetDirect3DDevice()->GetCopyableFootprints(&Desc, 0, 1, 0, &hardwareMemoryInfo.mLayout, &hardwareMemoryInfo.mRowCount, &hardwareMemoryInfo.mRowSizedInBytes, &memorySizeUsed);

	ID3D12Resource* tempbufferObject;
	D3D12_HEAP_PROPERTIES d3d12_heap_properties1 = InitD3D12HeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	GetDirect3DDevice()->CreateCommittedResource(
		&d3d12_heap_properties1,
		D3D12_HEAP_FLAG_NONE,
		&d3d12ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&tempbufferObject));
	tempbufferObject->SetName(L"Temp Vertex Buffer");

	BYTE* pData;
	HRESULT hr = tempbufferObject->Map(0, NULL, reinterpret_cast<void**>(&pData));
	if (FAILED(hr)) {
		return 0;
	}
	BYTE* pDstAGPVBO = reinterpret_cast<BYTE*>(pData + hardwareMemoryInfo.mLayout.Offset);
	const BYTE* pSrcCPUVertexData = reinterpret_cast<const BYTE*>(inData);
	for (UINT y = 0; y < hardwareMemoryInfo.mRowCount; ++y) {
		memcpy(pDstAGPVBO + hardwareMemoryInfo.mLayout.Footprint.RowPitch * y, pSrcCPUVertexData + hardwareMemoryInfo.mRowSizedInBytes * y, hardwareMemoryInfo.mRowSizedInBytes);
	}
	tempbufferObject->Unmap(0, NULL);

	if (tempbufferObject != nullptr) {
		inCommandList->CopyBufferRegion(bufferObject, 0, tempbufferObject, hardwareMemoryInfo.mLayout.Offset, hardwareMemoryInfo.mLayout.Footprint.Width);
		D3D12_RESOURCE_BARRIER barrier = InitResourceBarrier(bufferObject, D3D12_RESOURCE_STATE_COPY_DEST, inFinalResourceState);
		inCommandList->ResourceBarrier(1, &barrier);
	}
	return bufferObject;
}
ID3D12Resource* CreateTextureObject(DXGI_FORMAT inFormat,int inWidth,int inHeight,int inArraySize,int inMipmapLevelCount=1) {
	ID3D12Resource* texture = nullptr;
	D3D12_RESOURCE_DESC imageResourceDesc = {};
	imageResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	imageResourceDesc.Alignment = 0;
	imageResourceDesc.Width = inWidth;
	imageResourceDesc.Height = inHeight;
	imageResourceDesc.DepthOrArraySize = inArraySize;
	imageResourceDesc.MipLevels = inMipmapLevelCount;
	imageResourceDesc.Format = inFormat;
	imageResourceDesc.SampleDesc.Count = 1;
	imageResourceDesc.SampleDesc.Quality = 0;
	imageResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	imageResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES textureHeapProperties = {};
	textureHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	textureHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	textureHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	textureHeapProperties.CreationNodeMask = 1;
	textureHeapProperties.VisibleNodeMask = 1;

	HRESULT hr = sD3DDevice->CreateCommittedResource(
		&textureHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&imageResourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,//因为这里是临时内存的拷贝目标
		nullptr,
		IID_PPV_ARGS(&texture));
	if (FAILED(hr)) {
		return nullptr;
	}
	return texture;
}
D3D12_HEAP_PROPERTIES InitD3D12HeapProperties(D3D12_HEAP_TYPE inD3D12_HEAP_TYPE) {
	D3D12_HEAP_PROPERTIES d3d12HeapProperties = {
		inD3D12_HEAP_TYPE,D3D12_CPU_PAGE_PROPERTY_UNKNOWN,D3D12_MEMORY_POOL_UNKNOWN,1,1
	};
	return d3d12HeapProperties;
}
D3D12_BLEND_DESC InitDefaultBlendDesc() {
	D3D12_BLEND_DESC d3d12BlendDesc = { 0 };
	d3d12BlendDesc.AlphaToCoverageEnable = FALSE;
	d3d12BlendDesc.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
		FALSE,FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		d3d12BlendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
	return d3d12BlendDesc;
}
D3D12_RESOURCE_BARRIER InitResourceBarrier(ID3D12Resource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) {
	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	ZeroMemory(&d3dResourceBarrier, sizeof(d3dResourceBarrier));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = pResource;
	d3dResourceBarrier.Transition.StateBefore = stateBefore;
	d3dResourceBarrier.Transition.StateAfter = stateAfter;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	return d3dResourceBarrier;
}
ID3D12Resource* LoadHDRITextureFromFile(ID3D12GraphicsCommandList* inCommandList, const char* inFilePath, DXGI_FORMAT inFormat) {
	int image_width, image_height, channel_count;
	float* pixelData = stbi_loadf("Res/Image/1.hdr", &image_width, &image_height, &channel_count, 0);

	D3D12_RESOURCE_DESC imageResourceDesc = { };
	imageResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	imageResourceDesc.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
	imageResourceDesc.Width = image_width; // width of the texture
	imageResourceDesc.Height = image_height; // height of the texture
	imageResourceDesc.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
	imageResourceDesc.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
	imageResourceDesc.Format = inFormat; // This is the dxgi format of the image (format of the pixels)
	imageResourceDesc.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
	imageResourceDesc.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
	imageResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
	imageResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags

	ID3D12Resource* texture = nullptr;
	D3D12_HEAP_PROPERTIES textureHeapProperties = InitD3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT hr = GetDirect3DDevice()->CreateCommittedResource(
		&textureHeapProperties, // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&imageResourceDesc, // the description of our texture
		D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
		nullptr, // used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&texture));
	if (FAILED(hr)) {
		return nullptr;
	}
	texture->SetName(L"Texture2D");

	UINT64 textureUploadBufferSize;
	GetDirect3DDevice()->GetCopyableFootprints(&imageResourceDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);
	ID3D12Resource* tempBuffer;
	D3D12_HEAP_PROPERTIES tempHeapProperties = {};
	tempHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	tempHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	tempHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	tempHeapProperties.CreationNodeMask = 1;
	tempHeapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC tempResourceDesc = {};
	tempResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	tempResourceDesc.Alignment = 0;
	tempResourceDesc.Width = textureUploadBufferSize;
	tempResourceDesc.Height = 1;
	tempResourceDesc.DepthOrArraySize = 1;
	tempResourceDesc.MipLevels = 1;
	tempResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	tempResourceDesc.SampleDesc.Count = 1;
	tempResourceDesc.SampleDesc.Quality = 0;
	tempResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	tempResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	GetDirect3DDevice()->CreateCommittedResource(
		&tempHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&tempResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&tempBuffer));
	tempBuffer->SetName(L"Temp Texture Buffer");

	UINT64 memorySizeUsed = 0;
	D3DHardwareMemoryInfo textureMemoryInfo = { 0 };
	D3D12_RESOURCE_DESC Desc = texture->GetDesc();
	GetDirect3DDevice()->GetCopyableFootprints(&Desc, 0, 1, 0, &textureMemoryInfo.mLayout, &textureMemoryInfo.mRowCount, &textureMemoryInfo.mRowSizedInBytes, &memorySizeUsed);

	BYTE* tempBufferData;
	hr = tempBuffer->Map(0, NULL, reinterpret_cast<void**>(&tempBufferData));
	if (FAILED(hr)) {
		return nullptr;
	}
	BYTE* pDstAGPBuffer = reinterpret_cast<BYTE*>(tempBufferData + textureMemoryInfo.mLayout.Offset);
	for (UINT y = 0; y < textureMemoryInfo.mRowCount; ++y) {
		memcpy(pDstAGPBuffer + textureMemoryInfo.mLayout.Footprint.RowPitch * y, ((unsigned char*)pixelData) + textureMemoryInfo.mRowSizedInBytes * y, textureMemoryInfo.mRowSizedInBytes);
	}
	tempBuffer->Unmap(0, NULL);

	if (tempBuffer != nullptr) {
		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource = texture;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = 0;
		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = tempBuffer;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = textureMemoryInfo.mLayout;
		inCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	inCommandList->ResourceBarrier(1, &barrier);
	inFormat = imageResourceDesc.Format;
	stbi_image_free(pixelData);
	return texture;
}
ID3D12Resource* CreateTexture2D(ID3D12GraphicsCommandList* inCommandList, LPCTSTR inFilePath, DXGI_FORMAT& inFormat) {
	ImageData* imageData = LoadImageDataFromFile(inFilePath);
	D3D12_RESOURCE_DESC imageResourceDesc = { };
	imageResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	imageResourceDesc.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
	imageResourceDesc.Width = imageData->mImageWidth; // width of the texture
	imageResourceDesc.Height = imageData->mImageHeight; // height of the texture
	imageResourceDesc.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
	imageResourceDesc.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
	imageResourceDesc.Format = imageData->mDXGI_FORMAT; // This is the dxgi format of the image (format of the pixels)
	imageResourceDesc.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
	imageResourceDesc.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
	imageResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
	imageResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags

	ID3D12Resource* texture = nullptr;
	D3D12_HEAP_PROPERTIES textureHeapProperties = InitD3D12HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT hr = GetDirect3DDevice()->CreateCommittedResource(
		&textureHeapProperties, // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&imageResourceDesc, // the description of our texture
		D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
		nullptr, // used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&texture));
	if (FAILED(hr)) {
		return nullptr;
	}
	texture->SetName(L"Texture Buffer");

	UINT64 textureUploadBufferSize;
	GetDirect3DDevice()->GetCopyableFootprints(&imageResourceDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);
	ID3D12Resource* tempBuffer;
	D3D12_HEAP_PROPERTIES tempHeapProperties = {};
	tempHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	tempHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	tempHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	tempHeapProperties.CreationNodeMask = 1;
	tempHeapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC tempResourceDesc = {};
	tempResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	tempResourceDesc.Alignment = 0;
	tempResourceDesc.Width = textureUploadBufferSize;
	tempResourceDesc.Height = 1;
	tempResourceDesc.DepthOrArraySize = 1;
	tempResourceDesc.MipLevels = 1;
	tempResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	tempResourceDesc.SampleDesc.Count = 1;
	tempResourceDesc.SampleDesc.Quality = 0;
	tempResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	tempResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	GetDirect3DDevice()->CreateCommittedResource(
		&tempHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&tempResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&tempBuffer));
	tempBuffer->SetName(L"Temp Texture Buffer");

	UINT64 memorySizeUsed = 0;
	D3DHardwareMemoryInfo textureMemoryInfo = { 0 };
	D3D12_RESOURCE_DESC Desc = texture->GetDesc();
	GetDirect3DDevice()->GetCopyableFootprints(&Desc, 0, 1, 0, &textureMemoryInfo.mLayout, &textureMemoryInfo.mRowCount, &textureMemoryInfo.mRowSizedInBytes, &memorySizeUsed);

	BYTE* tempBufferData;
	hr = tempBuffer->Map(0, NULL, reinterpret_cast<void**>(&tempBufferData));
	if (FAILED(hr)) {
		return nullptr;
	}
	BYTE* pDstAGPBuffer = reinterpret_cast<BYTE*>(tempBufferData + textureMemoryInfo.mLayout.Offset);
	for (UINT y = 0; y < textureMemoryInfo.mRowCount; ++y) {
		memcpy(pDstAGPBuffer + textureMemoryInfo.mLayout.Footprint.RowPitch * y, imageData->mPixelData + textureMemoryInfo.mRowSizedInBytes * y, textureMemoryInfo.mRowSizedInBytes);
	}
	tempBuffer->Unmap(0, NULL);

	if (tempBuffer != nullptr) {
		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource = texture;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = 0;
		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = tempBuffer;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = textureMemoryInfo.mLayout;
		inCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	inCommandList->ResourceBarrier(1, &barrier);
	inFormat = imageResourceDesc.Format;
	return texture;
}
ID3D12Resource* CreateIntermediateBuffer(int inBufferSize) {
	ID3D12Resource* tempBuffer = nullptr;
	D3D12_HEAP_PROPERTIES tempBufferHeapProperties = {};
	tempBufferHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	tempBufferHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	tempBufferHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	tempBufferHeapProperties.CreationNodeMask = 1;
	tempBufferHeapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC tempResourceDesc = {};
	tempResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	tempResourceDesc.Alignment = 0;
	tempResourceDesc.Width = inBufferSize;
	tempResourceDesc.Height = 1;
	tempResourceDesc.DepthOrArraySize = 1;
	tempResourceDesc.MipLevels = 1;
	tempResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	tempResourceDesc.SampleDesc.Count = 1;
	tempResourceDesc.SampleDesc.Quality = 0;
	tempResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	tempResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	GetDirect3DDevice()->CreateCommittedResource(
		&tempBufferHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&tempResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&tempBuffer));
	tempBuffer->SetName(L"Temp Buffer");
	return tempBuffer;
}
inline void MemcpySubresource(
	_In_ const D3D12_MEMCPY_DEST* pDest,
	_In_ const D3D12_SUBRESOURCE_DATA* pSrc,
	SIZE_T RowSizeInBytes,
	UINT NumRows,
	UINT NumSlices)
{
	for (UINT z = 0; z < NumSlices; ++z)
	{
		BYTE* pDestSlice = reinterpret_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
		const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * z;
		for (UINT y = 0; y < NumRows; ++y)
		{
			memcpy(pDestSlice + pDest->RowPitch * y,
				pSrcSlice + pSrc->RowPitch * y,
				RowSizeInBytes);
		}
	}
}
ID3D12Resource* CreateTextureCube(ID3D12GraphicsCommandList* inCommandList, LPCTSTR* inFilePath) {
	ImageData* imageData = LoadImageDataFromFile(inFilePath[0]);
	const UINT num2DSubresources = 6;
	ID3D12Resource* textureCube = CreateTextureObject(imageData->mDXGI_FORMAT, imageData->mImageWidth, imageData->mImageHeight, num2DSubresources,1);
	textureCube->SetName(L"Texture Cube");
	D3D12_RESOURCE_DESC textureCubeDesc = textureCube->GetDesc();

	UINT64 tempMemorySize = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * num2DSubresources;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(new unsigned char[tempMemorySize]);
	UINT* pNumRows = reinterpret_cast<UINT*>(pLayouts + num2DSubresources);
	UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pNumRows + num2DSubresources);
	UINT64 textureUploadBufferSize;
	sD3DDevice->GetCopyableFootprints(&textureCubeDesc, 0, num2DSubresources, 0, pLayouts, pNumRows, pRowSizesInBytes, &textureUploadBufferSize);

	ID3D12Resource* tempBuffer = CreateIntermediateBuffer(textureUploadBufferSize);
	BYTE* tempBufferData;
	HRESULT hr = tempBuffer->Map(0, NULL, reinterpret_cast<void**>(&tempBufferData));
	if (FAILED(hr)) {
		return nullptr;
	}
	{
		D3D12_SUBRESOURCE_DATA srcData = {};
		srcData.pData = (const void*)imageData->mPixelData;
		srcData.RowPitch = static_cast<UINT>(textureCubeDesc.Width * 4);
		srcData.SlicePitch = static_cast<UINT>(textureCubeDesc.Width * textureCubeDesc.Height * 4);
		D3D12_MEMCPY_DEST DestData = { tempBufferData + pLayouts[0].Offset, pLayouts[0].Footprint.RowPitch, pLayouts[0].Footprint.RowPitch * pNumRows[0] };
		MemcpySubresource(&DestData, &srcData, (SIZE_T)pRowSizesInBytes[0], pNumRows[0], pLayouts[0].Footprint.Depth);
	}
	{
		for (UINT i = 1; i < num2DSubresources; ++i)
		{
			imageData = LoadImageDataFromFile(inFilePath[i]);
			D3D12_SUBRESOURCE_DATA srcData = {};
			srcData.pData = (const void*)imageData->mPixelData;
			srcData.RowPitch = static_cast<UINT>(textureCubeDesc.Width*4);
			srcData.SlicePitch = static_cast<UINT>(textureCubeDesc.Width * textureCubeDesc.Height * 4);
			D3D12_MEMCPY_DEST DestData = { tempBufferData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, pLayouts[i].Footprint.RowPitch * pNumRows[i] };
			MemcpySubresource(&DestData, &srcData, (SIZE_T)pRowSizesInBytes[i], pNumRows[i], pLayouts[i].Footprint.Depth);
		}
	}
	tempBuffer->Unmap(0, NULL);
	if (tempBuffer != nullptr) {
		for (UINT i = 0; i < num2DSubresources; ++i)
		{
			D3D12_TEXTURE_COPY_LOCATION dst = {};
			dst.pResource = textureCube;
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.SubresourceIndex = i;

			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.pResource = tempBuffer;
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint = pLayouts[i];

			inCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		}
	}

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = textureCube;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	inCommandList->ResourceBarrier(1, &barrier);
	//inFormat = imageResourceDesc.Format;
	delete[]((unsigned char*)pLayouts);
	return textureCube;
}
bool InitializeDirect3D12(HWND inHWND, int inWidth, int inHeight) {
	sViewportWidth = inWidth;
	sViewportHeight = inHeight;
	HRESULT hr;
	UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
	// 检查调试层是否可用
	{
		ID3D12Debug* debugController = nullptr;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif
	IDXGIFactory4* dxgiFactory;
	hr = CreateDXGIFactory2(dxgiFactoryFlags,IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		return false;
	}
	IDXGIAdapter1* adapter;
	int adapterIndex = 0;
	bool adapterFound = false;
	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}
		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr)) {
			adapterFound = true;
			break;
		}
		adapterIndex++;
	}
	if (!adapterFound) {
		return false;
	}
	hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&sD3DDevice));
	if (FAILED(hr)) {
		return false;
	}
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	hr = sD3DDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&sCommandQueue));
	if (FAILED(hr)) {
		return false;
	}
	DXGI_MODE_DESC backBufferDesc = {};
	backBufferDesc.Width = inWidth;
	backBufferDesc.Height = inHeight;
	backBufferDesc.Format = sColorRTFormat;
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = sDefaultFrameBufferCount;
	swapChainDesc.BufferDesc = backBufferDesc;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = inHWND;
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.Windowed = true;

	IDXGISwapChain* tempSwapChain;
	dxgiFactory->CreateSwapChain(sCommandQueue, &swapChainDesc, &tempSwapChain);
	sSwapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);

	sCurrentFrameIndex = sSwapChain->GetCurrentBackBufferIndex();

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = sDefaultFrameBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	hr = sD3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&sRTVDescriptorHeap));
	if (FAILED(hr)) {
		return false;
	}
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = sD3DDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&sDSDescriptorHeap));

	sRTVDescriptorSize = sD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvBaseHandle=sRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (int i = 0; i < sDefaultFrameBufferCount; i++) {
		hr = sSwapChain->GetBuffer(i, IID_PPV_ARGS(&sRenderTargets[i]));
		if (FAILED(hr)) {
			return false;
		}
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
		rtvHandle.ptr = rtvBaseHandle.ptr + i * sRTVDescriptorSize;
		sD3DDevice->CreateRenderTargetView(sRenderTargets[i], nullptr, rtvHandle);
	}
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = sDSRTFormat;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = sDSRTFormat;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES d3dHeapProperties = {};
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC d3dResourceDesc = {};
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = inWidth;
	d3dResourceDesc.Height = inHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 0;
	d3dResourceDesc.Format = sDSRTFormat;
	d3dResourceDesc.SampleDesc.Count = 1;
	d3dResourceDesc.SampleDesc.Quality = 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	sD3DDevice->CreateCommittedResource(
		&d3dHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&sDepthStencilBuffer)
	);
	sDSDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");
	sD3DDevice->CreateDepthStencilView(sDepthStencilBuffer, &depthStencilDesc, sDSDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	hr = sD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&sCommandAllocator));
	if (FAILED(hr)) {
		return false;
	}
	hr = sD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, sCommandAllocator, NULL, IID_PPV_ARGS(&sCommandList));
	if (FAILED(hr))
	{
		return false;
	}
	sCommandList->Close();

	hr = sD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&sFence));
	if (FAILED(hr)) {
		return false;
	}
	sFenceValue = 0;
	sFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (sFenceEvent == nullptr) {
		return false;
	}
	gRTV_DescriptorSize = sD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	gDSV_DescriptorSize = sD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	gCBV_SRV_UAV_DescriptorSize = sD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	gSAMPLER_DescriptorSize = sD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	Material::mRootSignature = InitializeRootSignature();
	return true;
}
ID3D12Device* GetDirect3DDevice() {
	return sD3DDevice;
}
void WaitForPreviousFrame() {
	if (sFence->GetCompletedValue() < sFenceValue) {
		sFence->SetEventOnCompletion(sFenceValue, sFenceEvent);
		WaitForSingleObject(sFenceEvent, INFINITE);
	}
	sFenceValue++;
	sCurrentFrameIndex = sSwapChain->GetCurrentBackBufferIndex();
}
FrameBufferRT* BeginRenderFrame(ID3D12GraphicsCommandList* inCommandList) {
	D3D12_RESOURCE_BARRIER barrier0 = InitResourceBarrier(sRenderTargets[sCurrentFrameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	inCommandList->ResourceBarrier(1, &barrier0);
	FrameBufferRT* frameBuffer = new FrameBufferRT;
	frameBuffer->mColorBuffer.mDescriptorHeap = sRTVDescriptorHeap;
	frameBuffer->mColorBuffer.mRTV.ptr = frameBuffer->mColorBuffer.mDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + sCurrentFrameIndex * GetDirect3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	frameBuffer->mColorBuffer.mFormat = sColorRTFormat;
	frameBuffer->mDSBuffer.mDescriptorHeap = sDSDescriptorHeap;
	frameBuffer->mDSBuffer.mRTV = frameBuffer->mDSBuffer.mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	frameBuffer->mDSBuffer.mFormat = sDSRTFormat;
	inCommandList->OMSetRenderTargets(1, &frameBuffer->mColorBuffer.mRTV, FALSE, &frameBuffer->mDSBuffer.mRTV);
	D3D12_VIEWPORT viewport = { 0.0f,0.0f,float(sViewportWidth),float(sViewportHeight),0.0f,1.0f };
	D3D12_RECT scissorRect = { 0,0,sViewportWidth,sViewportHeight };
	inCommandList->RSSetViewports(1, &viewport);
	inCommandList->RSSetScissorRects(1, &scissorRect);
	const float clearColor[] = { 0.1f, 0.4f, 0.6f, 1.0f };
	inCommandList->ClearRenderTargetView(frameBuffer->mColorBuffer.mRTV, clearColor, 0, nullptr);
	inCommandList->ClearDepthStencilView(frameBuffer->mDSBuffer.mRTV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	return frameBuffer;
}
void EndRenderFrame(ID3D12GraphicsCommandList* inCmdList) {
	D3D12_RESOURCE_BARRIER frameEndBerrier = InitResourceBarrier(sRenderTargets[sCurrentFrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	inCmdList->ResourceBarrier(1, &frameEndBerrier);
}
void Direct3DSwapBuffers() {
	sSwapChain->Present(0, 0);
}
void CleanUpDirect3D12() {
	WaitForPreviousFrame();
	CloseHandle(sFenceEvent);
}
ID3D12GraphicsCommandList* GetCommandList(ID3D12PipelineState* inPSO) {
	sCommandAllocator->Reset();
	sCommandList->Reset(sCommandAllocator, inPSO);
	return sCommandList;
}
void EndCommandList(int inDeltaFenceValue) {
	sCommandList->Close();
	ID3D12CommandList* ppCommandLists[] = { sCommandList };
	sCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	sFenceValue+=inDeltaFenceValue;
	sCommandQueue->Signal(sFence, sFenceValue);
}
RHICommandList::RHICommandList() {
	mCommandList = GetCommandList();
}
RHICommandList::~RHICommandList() {
	EndCommandList(1);
}
ID3D12DescriptorHeap* CreateDescriptorHeap(int inDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE inD3D12_DESCRIPTOR_HEAP_TYPE) {
	ID3D12DescriptorHeap* d3d12DescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors = inDescriptorCount;
	descriptorHeapDesc.Flags = (inD3D12_DESCRIPTOR_HEAP_TYPE==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || inD3D12_DESCRIPTOR_HEAP_TYPE==D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)?
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE ;
	descriptorHeapDesc.Type = inD3D12_DESCRIPTOR_HEAP_TYPE;
	GetDirect3DDevice()->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&d3d12DescriptorHeap));
	return d3d12DescriptorHeap;
}
ID3D12DescriptorHeap* CreateStandardVertexCBVDescriptorHeap() {
	ID3D12DescriptorHeap* d3d12DescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors = 3;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	GetDirect3DDevice()->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&d3d12DescriptorHeap));
	return d3d12DescriptorHeap;
}
ID3D12DescriptorHeap* CreateStandardPixelCBVDescriptorHeap() {
	ID3D12DescriptorHeap* d3d12DescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	HRESULT result=GetDirect3DDevice()->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&d3d12DescriptorHeap));
	if (result != S_OK) {
		printf("CreateStandardPixelCBVDescriptorHeap failed!\n");
	}
	return d3d12DescriptorHeap;
}
ID3D12DescriptorHeap* CreateStandardPixelSRVDescriptorHeap() {
	ID3D12DescriptorHeap* d3d12DescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors = 16;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	GetDirect3DDevice()->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&d3d12DescriptorHeap));
	return d3d12DescriptorHeap;
}
ID3D12Resource* GenConstantBuffer(int inDataLen) {
	ID3D12Resource* d3d12Resource;

	D3D12_HEAP_PROPERTIES d3d12_HEAP_PROPERTIES = {};
	d3d12_HEAP_PROPERTIES.Type = D3D12_HEAP_TYPE_UPLOAD;
	d3d12_HEAP_PROPERTIES.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3d12_HEAP_PROPERTIES.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3d12_HEAP_PROPERTIES.CreationNodeMask = 1;
	d3d12_HEAP_PROPERTIES.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC d3d12_RESOURCE_DESC;
	d3d12_RESOURCE_DESC.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	d3d12_RESOURCE_DESC.Alignment = 0;
	d3d12_RESOURCE_DESC.Width = inDataLen;
	d3d12_RESOURCE_DESC.Height = 1;
	d3d12_RESOURCE_DESC.DepthOrArraySize = 1;
	d3d12_RESOURCE_DESC.MipLevels = 1;
	d3d12_RESOURCE_DESC.Format = DXGI_FORMAT_UNKNOWN;
	d3d12_RESOURCE_DESC.SampleDesc.Count = 1;
	d3d12_RESOURCE_DESC.SampleDesc.Quality = 0;
	d3d12_RESOURCE_DESC.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	d3d12_RESOURCE_DESC.Flags = D3D12_RESOURCE_FLAG_NONE;

	GetDirect3DDevice()->CreateCommittedResource(
		&d3d12_HEAP_PROPERTIES, // this heap will be used to upload the constant buffer data
		D3D12_HEAP_FLAG_NONE, // no flags
		&d3d12_RESOURCE_DESC, // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
		D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
		nullptr, // we do not have use an optimized clear value for constant buffers
		IID_PPV_ARGS(&d3d12Resource));
	d3d12Resource->SetName(L"Constant Buffer");

	//D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	//cbvDesc.BufferLocation = d3d12Resource->GetGPUVirtualAddress();
	//cbvDesc.SizeInBytes = (sizeof(ConstantBuffer) + 255) & ~255;    // CB size is required to be 256-byte aligned.
	//
	//D3D12_CPU_DESCRIPTOR_HANDLE dstDescriptorPtr = inDescriptorStartPtr;
	//dstDescriptorPtr.ptr += inDescriptorIndex * gCBV_SRV_UAV_DescriptorSize;
	//GetDirect3DDevice()->CreateConstantBufferView(&cbvDesc, dstDescriptorPtr);

	return d3d12Resource;
}
void UpdateConstantBuffer(ID3D12Resource*inGPUConstantBuffer, void* inData, int inDataLen) {
	D3D12_RANGE d3d12Range = { 0 };
	d3d12Range.Begin = 0;
	d3d12Range.End = 0;
	UINT8* pData;
	inGPUConstantBuffer->Map(0, &d3d12Range, reinterpret_cast<void**>(&pData));
	memcpy(pData, inData, inDataLen);
	inGPUConstantBuffer->Unmap(0, NULL);
}
ID3D12PipelineState* GenPipelineStateObject(ID3D12RootSignature* inRootSignature, GPUProgram* inGPUProgram, D3D12_PRIMITIVE_TOPOLOGY_TYPE inD3D12_PRIMITIVE_TOPOLOGY_TYPE, 
	bool inEnableDepthTest, bool inEnableDepthWrite, DXGI_FORMAT inColorRTFormat, D3D12_CULL_MODE inCullMode, bool inFrontCounterClockwise) {
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = 3;
	inputLayoutDesc.pInputElementDescs = inputLayout;

	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	memcpy(&psoDesc.InputLayout, &inputLayoutDesc, sizeof(D3D12_INPUT_LAYOUT_DESC));
	psoDesc.pRootSignature = inRootSignature;
	psoDesc.VS = inGPUProgram->mVertexShader;
	psoDesc.PS = inGPUProgram->mFragmentShader;
	psoDesc.PrimitiveTopologyType = inD3D12_PRIMITIVE_TOPOLOGY_TYPE;
	psoDesc.RTVFormats[0] = inColorRTFormat;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc = sampleDesc;
	psoDesc.SampleMask = 0xffffffff;

	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = inCullMode;
	psoDesc.RasterizerState.FrontCounterClockwise = inFrontCounterClockwise;
	psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.RasterizerState.MultisampleEnable = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount = 0;
	psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	psoDesc.DepthStencilState.DepthEnable = inEnableDepthTest;
	psoDesc.DepthStencilState.DepthWriteMask = inEnableDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	psoDesc.BlendState = InitDefaultBlendDesc();
	psoDesc.NumRenderTargets = 1;

	ID3D12PipelineState* d3dPipelineState = nullptr;
	HRESULT hr = GetDirect3DDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&d3dPipelineState));
	if (FAILED(hr)) {
		switch (hr) {
		case E_INVALIDARG:
			printf("GenPipelineStateObject failed with E_INVALIDARG. Check your pipeline state description.\n");
			break;
		case E_OUTOFMEMORY:
			printf("GenPipelineStateObject failed with E_OUTOFMEMORY. Insufficient system memory.\n");
			break;
		default:
			printf("GenPipelineStateObject failed with HRESULT:\n");
			break;
		}
		return nullptr;
	}
	return d3dPipelineState;
}
ID3D12PipelineState* GenPipelineStateObject2(ID3D12RootSignature* inRootSignature, GPUProgram* inGPUProgram, D3D12_PRIMITIVE_TOPOLOGY_TYPE inD3D12_PRIMITIVE_TOPOLOGY_TYPE,
	bool inEnableDepthTest, bool inEnableDepthWrite, DXGI_FORMAT inColorRTFormat, D3D12_CULL_MODE inCullMode, bool inFrontCounterClockwise) {
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = 4;
	inputLayoutDesc.pInputElementDescs = inputLayout;

	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	memcpy(&psoDesc.InputLayout, &inputLayoutDesc, sizeof(D3D12_INPUT_LAYOUT_DESC));
	psoDesc.pRootSignature = inRootSignature;
	psoDesc.VS = inGPUProgram->mVertexShader;
	psoDesc.PS = inGPUProgram->mFragmentShader;
	psoDesc.PrimitiveTopologyType = inD3D12_PRIMITIVE_TOPOLOGY_TYPE;
	psoDesc.RTVFormats[0] = inColorRTFormat;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc = sampleDesc;
	psoDesc.SampleMask = 0xffffffff;

	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = inCullMode;
	psoDesc.RasterizerState.FrontCounterClockwise = inFrontCounterClockwise;
	psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.RasterizerState.MultisampleEnable = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount = 0;
	psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	psoDesc.DepthStencilState.DepthEnable = inEnableDepthTest;
	psoDesc.DepthStencilState.DepthWriteMask = inEnableDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	psoDesc.BlendState = InitDefaultBlendDesc();
	psoDesc.NumRenderTargets = 1;

	ID3D12PipelineState* d3dPipelineState = nullptr;
	HRESULT hr = GetDirect3DDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&d3dPipelineState));
	if (FAILED(hr)) {
		switch (hr) {
		case E_INVALIDARG:
			printf("GenPipelineStateObject failed with E_INVALIDARG. Check your pipeline state description.\n");
			break;
		case E_OUTOFMEMORY:
			printf("GenPipelineStateObject failed with E_OUTOFMEMORY. Insufficient system memory.\n");
			break;
		default:
			printf("GenPipelineStateObject failed with HRESULT:\n");
			break;
		}
		return nullptr;
	}
	return d3dPipelineState;
}