#pragma once
#include "Utils.h"
class Camera {
public:
	float mDistanceFromTarget;
	float mRotateAngle;
	DirectX::XMVECTOR mInitialViewDirection,mViewDirection,mPosition;
	DirectX::XMMATRIX mViewMatrix;
	void Init(DirectX::XMVECTOR inTargetPosition, float inDistanceFromTarget, DirectX::XMVECTOR inViewDirection);
	void Update(DirectX::XMVECTOR inTargetPosition,float inDistanceFromTarget,DirectX::XMVECTOR inViewDirection);
	void RoundRotate(float inDeltaTime, DirectX::XMVECTOR inTargetPosition, float inDistanceFromTarget, float inRotateSpeed);
};