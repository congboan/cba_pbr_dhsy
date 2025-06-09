#include "Camera.h"
void Camera::Init(DirectX::XMVECTOR inTargetPosition,float inDistanceFromTarget, DirectX::XMVECTOR inViewDirection) {
	inViewDirection= DirectX::XMVector3Normalize(inViewDirection);
	mInitialViewDirection = inViewDirection;
	mDistanceFromTarget = inDistanceFromTarget;
	mRotateAngle = 0.0;
	Update(inTargetPosition, inDistanceFromTarget, inViewDirection);
}
void Camera::Update(DirectX::XMVECTOR inTargetPosition, float inDistanceFromTarget, DirectX::XMVECTOR inViewDirection) {
	DirectX::XMVECTOR cameraPosition = DirectX::XMVectorSubtract(inTargetPosition, DirectX::XMVectorScale(inViewDirection, inDistanceFromTarget));
	DirectX::XMVECTOR rightDirection = DirectX::XMVector3Cross(inViewDirection, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	DirectX::XMVECTOR upDirection = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(rightDirection, inViewDirection));
	mPosition = cameraPosition;
	mViewDirection = inViewDirection;
	DirectX::XMVECTOR cameraFocus = DirectX::XMVectorAdd(cameraPosition, inViewDirection);
	mViewMatrix= DirectX::XMMatrixLookAtLH(cameraPosition, cameraFocus, upDirection);
}
void Camera::RoundRotate(float inDeltaTime, DirectX::XMVECTOR inTargetPosition,float inDistanceFromTarget, float inRotateSpeed) {
	float newAngle = mRotateAngle - inRotateSpeed * inDeltaTime;
	if (newAngle < -360.0f) {
		newAngle += 360.0f;
	}
	mRotateAngle = newAngle;
	DirectX::XMMATRIX rotateMatrix = DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(newAngle));

	DirectX::XMVECTOR inserseInitialViewDirection = DirectX::XMVectorSubtract(DirectX::XMVectorSet(0.0f,0.0f,0.0f,0.0f),mInitialViewDirection);
	DirectX::XMVECTOR inverseViewDirection = DirectX::XMVector3TransformNormal(inserseInitialViewDirection,rotateMatrix);
	inverseViewDirection = DirectX::XMVector3Normalize(inverseViewDirection);

	DirectX::XMVECTOR viewDirection = DirectX::XMVectorNegate(inverseViewDirection);
	Update(inTargetPosition, inDistanceFromTarget, viewDirection);
}