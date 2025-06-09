#include "utils.h"
#include "BattleFireDirect3D12.h"
#include "Mesh.h"
StaticMeshComponent::StaticMeshComponent() {
}
void StaticMeshComponent::SetPosition(int index, float x, float y, float z, float w) {
	mVertexData[index].mPosition.x = x;
	mVertexData[index].mPosition.y = y;
	mVertexData[index].mPosition.z = z;
	mVertexData[index].mPosition.w = w;
}
void StaticMeshComponent::SetTexcoord(int index, float x, float y, float z, float w) {
	mVertexData[index].mTexcoord.x = x;
	mVertexData[index].mTexcoord.y = y;
	mVertexData[index].mTexcoord.z = z;
	mVertexData[index].mTexcoord.w = w;
}
void StaticMeshComponent::SetNormal(int index, float x, float y, float z, float w) {
	mVertexData[index].mNormal.x = x;
	mVertexData[index].mNormal.y = y;
	mVertexData[index].mNormal.z = z;
	mVertexData[index].mNormal.w = w;
}
void StaticMeshComponent::InitFromFile(ID3D12GraphicsCommandList* inCmdList, const char* inFilePath) {
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, inFilePath, "rb");
	if (err == 0) {
		int temp = 0;
		fread(&temp, sizeof(int), 1, pFile);
		mVertexCount = temp;
		mVertexData = new StaticMeshVertexData[mVertexCount];
		mVertexDataSize = sizeof(StaticMeshVertexData);
		fread(mVertexData, sizeof(StaticMeshVertexData), mVertexCount, pFile);
		fread(&temp, sizeof(int), 1, pFile);
		char submeshName[256] = { 0 };
		fread(submeshName, 1, temp, pFile);
		fread(&temp, sizeof(int), 1, pFile);
		D3D12SubMesh* submesh = new D3D12SubMesh;
		submesh->mIndexCount = temp;
		submesh->mIndexes = new unsigned int[submesh->mIndexCount];
		fread(submesh->mIndexes, sizeof(unsigned int), submesh->mIndexCount, pFile);
		fclose(pFile);

		int indexBufferSize = sizeof(unsigned int) * submesh->mIndexCount;
		submesh->mIBO = GenBufferObject(inCmdList, submesh->mIndexes, indexBufferSize, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		submesh->mIBO->SetName(L"Index Buffer");
		submesh->mIBOView.BufferLocation = submesh->mIBO->GetGPUVirtualAddress();
		submesh->mIBOView.SizeInBytes = indexBufferSize;
		submesh->mIBOView.Format = DXGI_FORMAT_R32_UINT;

		mSubMeshes.insert(std::pair<std::string, D3D12SubMesh*>(submeshName,submesh));
	}
	int vertexBufferSize = sizeof(StaticMeshVertexData) * mVertexCount;
	mVBO = GenBufferObject(inCmdList, mVertexData, vertexBufferSize, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	mVBO->SetName(L"Vertex Buffer");

	mVBOView.BufferLocation = mVBO->GetGPUVirtualAddress();
	mVBOView.StrideInBytes = sizeof(StaticMeshVertexData);
	mVBOView.SizeInBytes = vertexBufferSize;

}
void StaticMeshComponent::InitFromFile2(ID3D12GraphicsCommandList* inCmdList, const char* inFilePath) {
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, inFilePath, "rb");
	if (err == 0) {
		int vertice_count;
		fread(&vertice_count, 1, sizeof(int), pFile);
		mVertexCount = vertice_count;
		mVertexData = new StaticMeshVertexDataEx[vertice_count];
		mVertexDataSize = sizeof(StaticMeshVertexDataEx);
		fread(mVertexData, 1, sizeof(StaticMeshVertexDataEx) * vertice_count, pFile);
		while (!feof(pFile)) {
			int nameLen = 0, indexCount = 0;
			fread(&nameLen, 1, sizeof(int), pFile);
			if (feof(pFile)) {
				break;
			}
			char name[256] = { 0 };
			fread(name, 1, sizeof(char) * nameLen, pFile);
			fread(&indexCount, 1, sizeof(int), pFile);
			D3D12SubMesh* submesh = new D3D12SubMesh;
			submesh->mIndexCount = indexCount;
			submesh->mIndexes = new unsigned int[indexCount];
			fread(submesh->mIndexes, 1, sizeof(unsigned int) * indexCount, pFile);

			int indexBufferSize = sizeof(unsigned int) * submesh->mIndexCount;
			submesh->mIBO = GenBufferObject(inCmdList, submesh->mIndexes, indexBufferSize, D3D12_RESOURCE_STATE_INDEX_BUFFER);
			submesh->mIBO->SetName(L"Index Buffer");
			submesh->mIBOView.BufferLocation = submesh->mIBO->GetGPUVirtualAddress();
			submesh->mIBOView.SizeInBytes = indexBufferSize;
			submesh->mIBOView.Format = DXGI_FORMAT_R32_UINT;

			mSubMeshes.insert(std::pair<std::string, D3D12SubMesh*>(name, submesh));
			printf("Load StaticMesh [%s] : vertex count[%d] submesh[%s] Index[%d]\n", inFilePath, vertice_count, name, indexCount);
		}
		fclose(pFile);
	}
	int vertexBufferSize = sizeof(StaticMeshVertexDataEx) * mVertexCount;
	mVBO = GenBufferObject(inCmdList, mVertexData, vertexBufferSize, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	mVBO->SetName(L"Vertex Buffer");

	mVBOView.BufferLocation = mVBO->GetGPUVirtualAddress();
	mVBOView.StrideInBytes = sizeof(StaticMeshVertexDataEx);
	mVBOView.SizeInBytes = vertexBufferSize;
}
void StaticMeshComponent::Render(ID3D12GraphicsCommandList* inCmdList) {
	D3D12_VERTEX_BUFFER_VIEW vertexBuffers[] = {
		mVBOView
	};
	inCmdList->IASetVertexBuffers(0, 1, vertexBuffers);
	if (mSubMeshes.empty()) {
		inCmdList->DrawInstanced(mVertexCount, 1, 0, 0);
	}
	else {
		std::unordered_map<std::string, D3D12SubMesh*>::iterator iter = mSubMeshes.begin();
		if (iter->second->mIBO != nullptr) {
			for (; iter != mSubMeshes.end(); iter++) {
				D3D12SubMesh* submesh = iter->second;
				inCmdList->IASetIndexBuffer(&submesh->mIBOView);
				inCmdList->DrawIndexedInstanced(submesh->mIndexCount, 1, 0, 0, 0);
			}
		}
	}
}
void FullScreenQuadMeshComponent::Init(ID3D12GraphicsCommandList* inCommandList) {
	mVertexCount = 4;
	mVertexData = new StaticMeshVertexData[4];
	mVertexDataSize = sizeof(StaticMeshVertexData);
	SetPosition(0, -1.0f, -1.0f, 0.0f);
	SetPosition(1, 1.0f, -1.0f, 0.0f);
	SetPosition(2, -1.0f, 1.0f, 0.0f);
	SetPosition(3, 1.0f, 1.0f, 0.0f);

	SetTexcoord(0, 0.0f, 1.0f);
	SetTexcoord(1, 1.0f, 1.0f);
	SetTexcoord(2, 0.0f, 0.0f);
	SetTexcoord(3, 1.0f, 0.0f);

	int vertexBufferSize = sizeof(StaticMeshVertexData) * mVertexCount;
	mVBO = GenBufferObject(inCommandList, mVertexData, vertexBufferSize, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	mVBO->SetName(L"FSQVertexBuffer");

	mVBOView.BufferLocation = mVBO->GetGPUVirtualAddress();
	mVBOView.StrideInBytes = sizeof(StaticMeshVertexData);
	mVBOView.SizeInBytes = vertexBufferSize;
}
SkinedMeshSubMesh::SkinedMeshSubMesh() {
}
static ID3D12RootSignature* InitializeSkinedMeshRootSignature() {
	D3D12_DESCRIPTOR_RANGE  cbDescriptorRanges[1]; // only one range right now
	cbDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // this is a range of constant buffer views (descriptors)
	cbDescriptorRanges[0].NumDescriptors = 1; // we only have one constant buffer, so the range is only 1
	cbDescriptorRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
	cbDescriptorRanges[0].RegisterSpace = 0; // space 0. can usually be zero
	cbDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables

	D3D12_DESCRIPTOR_RANGE  srvDescriptorRanges[1]; // only one range right now
	srvDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // this is a range of shader resource views (descriptors)
	srvDescriptorRanges[0].NumDescriptors = 1; // we only have one texture right now, so the range is only 1
	srvDescriptorRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
	srvDescriptorRanges[0].RegisterSpace = 0; // space 0. can usually be zero
	srvDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables

	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;
	rootCBVDescriptor.ShaderRegister = 0;

	D3D12_ROOT_DESCRIPTOR_TABLE cbDescriptorTable;
	cbDescriptorTable.NumDescriptorRanges = _countof(cbDescriptorRanges); // we only have one range
	cbDescriptorTable.pDescriptorRanges = &cbDescriptorRanges[0]; // the pointer to the beginning of our ranges array

	D3D12_ROOT_DESCRIPTOR_TABLE srvDescriptorTable;
	srvDescriptorTable.NumDescriptorRanges = _countof(srvDescriptorRanges); // we only have one range
	srvDescriptorTable.pDescriptorRanges = &srvDescriptorRanges[0]; // the pointer to the beginning of our ranges array

	D3D12_ROOT_PARAMETER  rootParameters[2]; // only one parameter right now
	/*rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
	rootParameters[0].DescriptorTable = cbDescriptorTable; // this is our descriptor table for this root parameter
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now*/

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
	rootParameters[0].Descriptor = rootCBVDescriptor; // this is the root descriptor for this root parameter
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
	rootParameters[1].DescriptorTable = srvDescriptorTable; // this is our descriptor table for this root parameter
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // our pixel shader will be the only shader accessing this parameter */


	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &sampler;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

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
void GenSkinedMeshPipelineStateObject(PipelineStateObject* inPipelineStateObject) {
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = 5;
	inputLayoutDesc.pInputElementDescs = inputLayout;

	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	D3D12_DEPTH_STENCIL_DESC d3d12DSDesc = {};
	d3d12DSDesc.DepthEnable = TRUE;
	d3d12DSDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	d3d12DSDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	d3d12DSDesc.StencilEnable = FALSE;
	d3d12DSDesc.StencilReadMask = FALSE;
	d3d12DSDesc.StencilWriteMask = FALSE;
	
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayoutDesc;
	//psoDesc.pRootSignature = inPipelineStateObject->mRootSignature;
	//psoDesc.VS = inPipelineStateObject->mGPUProgram->mVertexShader;
	//psoDesc.PS = inPipelineStateObject->mGPUProgram->mFragmentShader;
	//psoDesc.PrimitiveTopologyType = inPipelineStateObject->mPrimitiveType;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = sampleDesc;
	psoDesc.SampleMask = 0xffffffff;

	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;
	psoDesc.RasterizerState.MultisampleEnable = FALSE;
	psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	psoDesc.RasterizerState.ForcedSampleCount = 0;
	psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	psoDesc.DepthStencilState = d3d12DSDesc;
	psoDesc.BlendState = InitDefaultBlendDesc();
	psoDesc.NumRenderTargets = 1;

	// create the pso
	HRESULT hr = GetDirect3DDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&inPipelineStateObject->mPSO));
	if (FAILED(hr)) {
		return;
	}
}
void SkinedMeshComponent::InitFromFile(ID3D12GraphicsCommandList* inCmdList,const char* inFilePath) {
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, inFilePath, "rb");
	if (err == 0) {
		fread(&mFrameRate, 1, sizeof(int), pFile);
		fread(&mFrameCount, 1, sizeof(int), pFile);
		fread(&mBoneCount, 1, sizeof(int), pFile);
		fread(&mMeshCount, 1, sizeof(int), pFile);
		for (int meshIndex = 0; meshIndex < mMeshCount; ++meshIndex) {
			SkinedMeshSubMesh* mesh = new SkinedMeshSubMesh;
			mesh->mPipelineStateObject = new PipelineStateObject;
			//mesh->mPipelineStateObject->mRootSignature = InitializeSkinedMeshRootSignature();

			fread(&mesh->mVertexCount, sizeof(int), 1, pFile);
			mesh->mVertexData = new SkinedMeshVertexData[mesh->mVertexCount];
			fread(mesh->mVertexData, sizeof(SkinedMeshVertexData), mesh->mVertexCount, pFile);
			fread(&mesh->mIndexCount, sizeof(int), 1, pFile);
			mesh->mIndexes = new unsigned int[mesh->mIndexCount];
			fread(mesh->mIndexes, 1, sizeof(unsigned int) * mesh->mIndexCount, pFile);

			DXGI_FORMAT dxformat = DXGI_FORMAT_UNKNOWN;
			mesh->mTexture = CreateTexture2D(inCmdList, L"Res/LuoKeRen.png", dxformat);

			D3D12_DESCRIPTOR_HEAP_DESC srvDescriptorHeapDesc = {};
			srvDescriptorHeapDesc.NumDescriptors = 1;
			srvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			srvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			HRESULT hr = GetDirect3DDevice()->CreateDescriptorHeap(&srvDescriptorHeapDesc, IID_PPV_ARGS(&mesh->mTextureDescriptorHeap));

			// now we create a shader resource view (descriptor that points to the texture and describes it)
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = dxformat;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			GetDirect3DDevice()->CreateShaderResourceView(mesh->mTexture, &srvDesc, mesh->mTextureDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

			//mesh->mPipelineStateObject->mGPUProgram = GPUProgram::GetGPUProgram(L"Res/vs.hlsl", L"Res/fs.hlsl");
			GenSkinedMeshPipelineStateObject(mesh->mPipelineStateObject);


			int vertexBufferSize = sizeof(SkinedMeshVertexData) * mesh->mVertexCount;
			int indexBufferSize = sizeof(unsigned int) * mesh->mIndexCount;
			mesh->mVBO = GenBufferObject(inCmdList, mesh->mVertexData, vertexBufferSize, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			mesh->mVBO->SetName(L"Vertex Buffer");

			mesh->mIBO = GenBufferObject(inCmdList, mesh->mIndexes, indexBufferSize, D3D12_RESOURCE_STATE_INDEX_BUFFER);
			mesh->mIBO->SetName(L"Index Buffer");

			mesh->mVBOView.BufferLocation = mesh->mVBO->GetGPUVirtualAddress();
			mesh->mVBOView.StrideInBytes = sizeof(SkinedMeshVertexData);
			mesh->mVBOView.SizeInBytes = vertexBufferSize;

			mesh->mIBOView.BufferLocation = mesh->mIBO->GetGPUVirtualAddress();
			mesh->mIBOView.SizeInBytes = indexBufferSize;
			mesh->mIBOView.Format = DXGI_FORMAT_R32_UINT;

			mSubMeshes.push_back(mesh);
		}
		//tpose bone inverse matrix
		fread(mTPoseInverse, sizeof(float)*16, mBoneCount, pFile);
		for (int i = 0; i < mFrameCount; ++i) {
			AnimationFrame* animationFrame = new AnimationFrame;
			fread(animationFrame->mBones, sizeof(float) * 16, mBoneCount, pFile);
			mAnimationFrames.push_back(animationFrame);
		}
		mAnimationTime = 0.0f;
		mCurrentAnimationFrame = mAnimationFrames[0];
		fclose(pFile);
	}
}
void SkinedMeshSubMesh::Render(ID3D12GraphicsCommandList* inCmdList) {
	D3D12_VERTEX_BUFFER_VIEW vertexBuffers[] = {
		mVBOView
	};
	inCmdList->IASetVertexBuffers(0, 1, vertexBuffers);
	if (mIBO == nullptr){
		inCmdList->DrawInstanced(mVertexCount, 1, 0, 0);
	}
	else {
		inCmdList->IASetIndexBuffer(&mIBOView);
		inCmdList->DrawIndexedInstanced(mIndexCount, 1, 0, 0, 0);
	}
}
void SkinedMeshComponent::Update(float inDeltaTime) {
	static int frame_index = 0;
	static float frameTime = 1.0f / (float)mFrameCount;
	if (false == mPlayAnimation) {
		return;
	}
	mAnimationTime += inDeltaTime;
	while (mAnimationTime > frameTime) {
		mAnimationTime -= frameTime;
		frame_index++;
	}
	while (frame_index >= mFrameCount) {
		frame_index -= mFrameCount;
	}
	mCurrentAnimationFrame = mAnimationFrames[frame_index];
}