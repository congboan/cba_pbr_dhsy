#include "Node.h"
#include "Camera.h"
#include "Material.h"
Node::Node() {
	mStaticMeshComponent = nullptr; 
	mMat4UniformBufferData = new Mat4UniformBufferData(1024);
	mModelMatrix = DirectX::XMMatrixIdentity();
}
void Node::UpdateConstantBufferData(DirectX::XMMATRIX& inProjectionMatrix, DirectX::XMMATRIX& inViewMatrix) {
	mMat4UniformBufferData->SetMat4(0, &inProjectionMatrix);
	mMat4UniformBufferData->SetMat4(1, &inViewMatrix);
}
void Node::SetPosition(float inX, float inY, float inZ) {
	mModelMatrix = DirectX::XMMatrixTranslation(inX, inY, inZ);
}
void Node::Draw(ID3D12GraphicsCommandList* inCommandList, DirectX::XMMATRIX& inProjectionMatrix, Camera& inCamera, DXGI_FORMAT inColorRTFormat, DXGI_FORMAT inDSRTFormat) {
	DirectX::XMFLOAT4X4 tempMatrix;
	XMStoreFloat4x4(&tempMatrix, inProjectionMatrix);
	mMat4UniformBufferData->SetMat4(0, &tempMatrix);
	XMStoreFloat4x4(&tempMatrix, inCamera.mViewMatrix);
	mMat4UniformBufferData->SetMat4(1, &tempMatrix);
	XMStoreFloat4x4(&tempMatrix, mModelMatrix);
	mMat4UniformBufferData->SetMat4(2, &tempMatrix);
	DirectX::XMVECTOR determinant;
	DirectX::XMMATRIX inverseMatrix = DirectX::XMMatrixInverse(&determinant, mModelMatrix);
	if (DirectX::XMVectorGetX(determinant) != 0.0f) {
		DirectX::XMMATRIX transposedInverseMatrix = DirectX::XMMatrixTranspose(inverseMatrix);
		XMStoreFloat4x4(&tempMatrix, transposedInverseMatrix);
		mMat4UniformBufferData->SetMat4(3, &tempMatrix);
	}
	mMat4UniformBufferData->UpdateGPUData();
	if (mStaticMeshComponent != nullptr) {
		mStaticMeshComponent->mMaterial->Active(inCommandList,mStaticMeshComponent->mVertexDataSize,inColorRTFormat,inDSRTFormat);
		if (mStaticMeshComponent->mMaterial->mPipelineState != nullptr) {
			inCommandList->SetGraphicsRootConstantBufferView(0, mMat4UniformBufferData->mResource->GetGPUVirtualAddress());
			mStaticMeshComponent->Render(inCommandList);
		}
	}

	/*ID3D12DescriptorHeap* descriptorHeaps[] = {
		sGroundNode->mStaticMeshComponent->mPipelineStateObject->mVertexCBVDescriptorHeap
		//,sGroundNode->mStaticMeshComponent->mPipelineStateObject->mPixelCBVDescriptorHeap
		,sGroundNode->mStaticMeshComponent->mPipelineStateObject->mPixelSRVDescriptorHeap
	};
	UINT descriptorHeapCount = _countof(descriptorHeaps);
	cmdList->SetDescriptorHeaps(descriptorHeapCount, descriptorHeaps);*/
	//cmdList->SetGraphicsRootConstantBufferView(1, sGroundNode->mStaticMeshComponent->mPipelineStateObject->mCBs[1]->GetGPUVirtualAddress());
	//cmdList->SetGraphicsRootConstantBufferView(2, sGroundNode->mStaticMeshComponent->mPipelineStateObject->mCBs[2]->GetGPUVirtualAddress());
	//cmdList->SetGraphicsRootDescriptorTable(0, sGroundNode->mStaticMeshComponent->mPipelineStateObject->mVertexCBVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	//cmdList->SetGraphicsRootDescriptorTable(1, sGroundNode->mStaticMeshComponent->mPipelineStateObject->mPixelCBVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	
}