#include "Scene.h"
#include "Mesh.h"
#include "Node.h"
#include "Camera.h"
#include "FrameBuffer.h"
#include "BoxCapture.h"

static bool gMatrixInited = false;
static DirectX::XMMATRIX gCaptureProjectionMatrix;
static Camera gCaptureCameras[6];
/*void CopyRTImageToCubeMap(VkCommandBuffer inCommandBuffer, VkImage inSrcImage, int inWidth, int inHeight, VkImage inDstCubeMap, int inFace, int inMipmapLevel) {
	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT ,0,1,0,1 };
	TransferImageLayout(inCommandBuffer, inSrcImage, subresourceRange,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	VkImageCopy copyRegion = {};

	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.baseArrayLayer = 0;
	copyRegion.srcSubresource.mipLevel = 0;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.srcOffset = { 0, 0, 0 };

	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.dstSubresource.baseArrayLayer = inFace;
	copyRegion.dstSubresource.mipLevel = inMipmapLevel;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.dstOffset = { 0, 0, 0 };

	copyRegion.extent.width = static_cast<uint32_t>(inWidth);
	copyRegion.extent.height = static_cast<uint32_t>(inHeight);
	copyRegion.extent.depth = 1;

	vkCmdCopyImage(
		inCommandBuffer,
		inSrcImage,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		inDstCubeMap,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&copyRegion);
}
Texture* CreateTextureFromFile(const char* path) {
	int image_width, image_height, channel_count;
	float* pixelData = stbi_loadf(path, &image_width, &image_height, &channel_count, 0);
	float* pixel = new float[image_width*image_height*4];
	for (int i=0;i<image_width*image_height;i++){
		pixel[i * 4] = pixelData[i * 3];
		pixel[i * 4 + 1] = pixelData[i * 3 + 1];
		pixel[i * 4 + 2] = pixelData[i * 3 + 2];
		pixel[i * 4 + 3] = 1.0f;
	}
	Texture* texture = new Texture(VK_FORMAT_R32G32B32A32_SFLOAT);
	texture->mFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	GenImage(texture, image_width, image_height, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT);
	SubmitImage2D(texture, image_width, image_height, pixel);
	texture->mImageView = GenImageView2D(texture->mImage, texture->mFormat, texture->mImageAspectFlag);
	stbi_image_free(pixelData);
	delete[]pixel;
	return texture;
}*/
void InitMatrices() {
	if (gMatrixInited) {
		return;
	}
	gMatrixInited = true;
	gCaptureProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH((90.0f * 3.14f) / 180.0f, 1.0f, 0.1f, 100.0f);

	DirectX::XMVECTOR center= DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

	// Look along each coordinate axis.
	DirectX::XMVECTOR targets[6] =
	{
		DirectX::XMVectorSet(1.0f, 0.0f, 0.0f,1.0f), // +X
		DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f,1.0f), // -X
		DirectX::XMVectorSet(0.0f, 1.0f, 0.0f,1.0f), // +Y
		DirectX::XMVectorSet(0.0f, -1.0f, 0.0f,1.0f), // -Y
		DirectX::XMVectorSet(0.0f, 0.0f, 1.0f,1.0f), // +Z
		DirectX::XMVectorSet(0.0f, 0.0f, - 1.0f,1.0f)  // -Z
	};

	// Use world up vector (0,1,0) for all directions except +Y/-Y.  In these cases, we
	// are looking down +Y or -Y, so we need a different "up" vector.
	DirectX::XMVECTOR ups[6] =
	{
		DirectX::XMVectorSet(0.0f, 1.0f, 0.0f,0.0f),  // +X
		DirectX::XMVectorSet(0.0f, 1.0f, 0.0f,0.0f),  // -X
		DirectX::XMVectorSet(0.0f, 0.0f, -1.0f,0.0f), // +Y
		DirectX::XMVectorSet(0.0f, 0.0f, +1.0f,0.0f), // -Y
		DirectX::XMVectorSet(0.0f, 1.0f, 0.0f,0.0f),	 // +Z
		DirectX::XMVectorSet(0.0f, 1.0f, 0.0f,0.0f)	 // -Z
	};
	for (int i = 0; i < 6; ++i)
	{
		gCaptureCameras[i].mViewMatrix=DirectX::XMMatrixLookAtLH(center, targets[i], ups[i]);
	}
}
ID3D12Resource* HDRI2CubeMap(ID3D12GraphicsCommandList* inCommandList, ID3D12Resource* inHDRITexture, int inCubeMapResolution, LPCTSTR inVSFilePath, LPCTSTR inFSFilePath) {
	InitMatrices();
	StaticMeshComponent* skyBoxMesh = new StaticMeshComponent;
	skyBoxMesh->InitFromFile(inCommandList, "Res/Model/skybox.staticmesh");
	Node*node[6];
	for (int i = 0; i < 6; i++) {
		node[i] = new Node;
		node[i]->mStaticMeshComponent = skyBoxMesh;
		Material* material = new Material(inVSFilePath, inFSFilePath);
		node[i]->mStaticMeshComponent->mMaterial = material;
		material->SetCullMode(D3D12_CULL_MODE_NONE);
		material->EnableDepthTest(false);
		material->SetTexture(0, DXGI_FORMAT_R32G32B32_FLOAT, inHDRITexture);
	}

	BoxCapture*boxCapture = new BoxCapture;
	boxCapture->Init(inCubeMapResolution, DXGI_FORMAT_R32G32B32A32_FLOAT, 1);

	D3D12_RESOURCE_BARRIER berrierCRT = InitResourceBarrier(boxCapture->mCubeMapBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	inCommandList->ResourceBarrier(1, &berrierCRT);
	for (int i = 0; i < 6; i++) {
		D3D12_VIEWPORT viewport = { 0.0f,0.0f,float(boxCapture->mSize),float(boxCapture->mSize),0.0f,1.0f };
		D3D12_RECT scissorRect = { 0,0,boxCapture->mSize,boxCapture->mSize };
		inCommandList->OMSetRenderTargets(1, &boxCapture->mRTV[i][0], FALSE, &boxCapture->mDRTV[i][0]); 
		inCommandList->RSSetViewports(1, &viewport);
		inCommandList->RSSetScissorRects(1, &scissorRect);
		const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		inCommandList->ClearRenderTargetView(boxCapture->mRTV[i][0], clearColor, 0, nullptr);
		inCommandList->ClearDepthStencilView(boxCapture->mDRTV[i][0], D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		node[i]->Draw(inCommandList, gCaptureProjectionMatrix, gCaptureCameras[i], boxCapture->mFormat, DXGI_FORMAT_D24_UNORM_S8_UINT);
	}
	D3D12_RESOURCE_BARRIER berrierCRTEnd = InitResourceBarrier(boxCapture->mCubeMapBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	inCommandList->ResourceBarrier(1, &berrierCRTEnd);
	return boxCapture->mCubeMapBuffer;
}
ID3D12Resource* LoadHDRICubeMapFromFile(ID3D12GraphicsCommandList* inCommandList, const char* inFilePath, int inCubeMapResolution, LPCTSTR inVSFilePath, LPCTSTR inFSFilePath) {
	ID3D12Resource* hdriTexture2D = LoadHDRITextureFromFile(inCommandList, inFilePath, DXGI_FORMAT_R32G32B32_FLOAT);
	ID3D12Resource* hdriCubeMap = HDRI2CubeMap(inCommandList, hdriTexture2D, inCubeMapResolution, inVSFilePath,inFSFilePath);
	return hdriCubeMap;
}
ID3D12Resource* CaptureDiffuseIrradiance(ID3D12GraphicsCommandList* inCommandList, ID3D12Resource* inSrcCubeMap, int inCubeMapResolution, LPCTSTR inVSFilePath, LPCTSTR inFSFilePath) {
	BoxCapture* boxCapture = new BoxCapture;
	boxCapture->Init(inCubeMapResolution, DXGI_FORMAT_R32G32B32A32_FLOAT, 1);
	StaticMeshComponent* skyBoxMesh = new StaticMeshComponent;
	skyBoxMesh->InitFromFile(inCommandList, "Res/Model/skybox.staticmesh");
	Node* node[6];
	for (int i = 0; i < 6; i++) {
		node[i] = new Node;
		node[i]->mStaticMeshComponent = skyBoxMesh;
		Material* material = new Material(inVSFilePath, inFSFilePath);
		node[i]->mStaticMeshComponent->mMaterial = material;
		material->SetCullMode(D3D12_CULL_MODE_NONE);
		material->EnableDepthTest(false);
		material->SetTextureCube(0, DXGI_FORMAT_R32G32B32A32_FLOAT, inSrcCubeMap);
	}
	D3D12_RESOURCE_BARRIER berrierCRT = InitResourceBarrier(boxCapture->mCubeMapBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	inCommandList->ResourceBarrier(1, &berrierCRT);
	for (int i = 0; i < 6; i++) {
		D3D12_VIEWPORT viewport = { 0.0f,0.0f,float(boxCapture->mSize),float(boxCapture->mSize),0.0f,1.0f };
		D3D12_RECT scissorRect = { 0,0,boxCapture->mSize,boxCapture->mSize };
		inCommandList->OMSetRenderTargets(1, &boxCapture->mRTV[i][0], FALSE, &boxCapture->mDRTV[i][0]);
		inCommandList->RSSetViewports(1, &viewport);
		inCommandList->RSSetScissorRects(1, &scissorRect);
		const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		inCommandList->ClearRenderTargetView(boxCapture->mRTV[i][0], clearColor, 0, nullptr);
		inCommandList->ClearDepthStencilView(boxCapture->mDRTV[i][0], D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		node[i]->Draw(inCommandList, gCaptureProjectionMatrix, gCaptureCameras[i], boxCapture->mFormat, DXGI_FORMAT_D24_UNORM_S8_UINT);
	}
	D3D12_RESOURCE_BARRIER berrierCRTEnd = InitResourceBarrier(boxCapture->mCubeMapBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	inCommandList->ResourceBarrier(1, &berrierCRTEnd);
	return boxCapture->mCubeMapBuffer;
}
ID3D12Resource* CapturePrefilteredColor(ID3D12GraphicsCommandList* inCommandList, ID3D12Resource* inSrcCubeMap, int inCubeMapResolution, LPCTSTR inVSFilePath, LPCTSTR inFSFilePath) {
	BoxCapture* boxCapture = new BoxCapture;
	boxCapture->Init(inCubeMapResolution, DXGI_FORMAT_R32G32B32A32_FLOAT, 5);
	StaticMeshComponent* skyBoxMesh = new StaticMeshComponent;
	skyBoxMesh->InitFromFile(inCommandList, "Res/Model/skybox.staticmesh");
	Vec4UniformBufferData* vec4UBOs[5];
	for (int mipmapLevel = 0; mipmapLevel < 5; mipmapLevel++) {
		float roughness = float(mipmapLevel) / float(4);
		vec4UBOs[mipmapLevel] = new Vec4UniformBufferData(1024);
		float v[] = {roughness,0.0f,0.0f,0.0f};
		vec4UBOs[mipmapLevel]->SetVec4(0, v);
		vec4UBOs[mipmapLevel]->UpdateGPUData();
	}
	Node* node[6];
	for (int i = 0; i < 6; i++) {
		node[i] = new Node;
		node[i]->mStaticMeshComponent = skyBoxMesh;
		Material* material = new Material(inVSFilePath, inFSFilePath);
		node[i]->mStaticMeshComponent->mMaterial = material;
		material->SetCullMode(D3D12_CULL_MODE_NONE);
		material->EnableDepthTest(false);
		material->SetTextureCube(0, DXGI_FORMAT_R32G32B32A32_FLOAT, inSrcCubeMap);
	}
	D3D12_RESOURCE_BARRIER berrierCRT = InitResourceBarrier(boxCapture->mCubeMapBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	inCommandList->ResourceBarrier(1, &berrierCRT);

	for (int mipmapLevel = 0; mipmapLevel < 5; mipmapLevel++) {
		for (int i = 0; i < 6; i++) {
			float currentResolution = static_cast<float>(inCubeMapResolution * std::pow(0.5f, mipmapLevel));
			D3D12_VIEWPORT viewport = { 0.0f,0.0f,currentResolution,currentResolution,0.0f,1.0f };
			D3D12_RECT scissorRect = { 0,0,int(currentResolution),int(currentResolution) };
			node[i]->mStaticMeshComponent->mMaterial->mVec4s = vec4UBOs[mipmapLevel];
			inCommandList->OMSetRenderTargets(1, &boxCapture->mRTV[i][mipmapLevel], FALSE, &boxCapture->mDRTV[i][mipmapLevel]);
			inCommandList->RSSetViewports(1, &viewport);
			inCommandList->RSSetScissorRects(1, &scissorRect);
			const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			inCommandList->ClearRenderTargetView(boxCapture->mRTV[i][mipmapLevel], clearColor, 0, nullptr);
			inCommandList->ClearDepthStencilView(boxCapture->mDRTV[i][mipmapLevel], D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
			node[i]->Draw(inCommandList, gCaptureProjectionMatrix, gCaptureCameras[i], boxCapture->mFormat, DXGI_FORMAT_D24_UNORM_S8_UINT);
		}
	}
	D3D12_RESOURCE_BARRIER berrierCRTEnd = InitResourceBarrier(boxCapture->mCubeMapBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	inCommandList->ResourceBarrier(1, &berrierCRTEnd);
	return boxCapture->mCubeMapBuffer;
}
ID3D12Resource* GenerateBRDF(ID3D12GraphicsCommandList* inCommandList, int inResolution, LPCTSTR inVSFilePath , LPCTSTR inFSFilePath) {
	Node*fsqNode = new Node;
	FullScreenQuadMeshComponent* fsq = new FullScreenQuadMeshComponent;
	fsq->Init(inCommandList);
	fsqNode->mStaticMeshComponent = fsq;
	Material* material = new Material(inVSFilePath, inFSFilePath);
	material->SetPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	material->EnableDepthTest(false);
	fsqNode->mStaticMeshComponent->mMaterial = material;

	FrameBuffer*fbo = new FrameBuffer;
	fbo->SetSize(inResolution, inResolution);
	fbo->AttachColorBuffer(DXGI_FORMAT_R32G32_FLOAT);
	fbo->AttachDepthBuffer();

	D3D12_VIEWPORT viewport = { 0.0f,0.0f,float(inResolution),float(inResolution),0.0f,1.0f};
	D3D12_RECT scissorRect = { 0,0,inResolution,inResolution };
	FrameBufferRT* frameBuffer = fbo->BeginRendering(inCommandList);
	inCommandList->OMSetRenderTargets(1, &frameBuffer->mColorBuffer.mRTV, FALSE, &frameBuffer->mDSBuffer.mRTV);
	const float hdrClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	inCommandList->ClearRenderTargetView(frameBuffer->mColorBuffer.mRTV, hdrClearColor, 0, nullptr);
	inCommandList->ClearDepthStencilView(frameBuffer->mDSBuffer.mRTV, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	inCommandList->RSSetViewports(1, &viewport);
	inCommandList->RSSetScissorRects(1, &scissorRect);
	fsqNode->Draw(inCommandList, gCaptureProjectionMatrix, gCaptureCameras[0], frameBuffer->mColorBuffer.mFormat, frameBuffer->mDSBuffer.mFormat);
	fbo->EndRendering(inCommandList);
	delete frameBuffer;
	return fbo->mColorBuffer;
}