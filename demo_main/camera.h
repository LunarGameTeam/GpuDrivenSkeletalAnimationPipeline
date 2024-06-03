#pragma once
#include"gpu_resource_helper.h"
class SimpleCamera
{
	DirectX::XMFLOAT3 camera_right;
	DirectX::XMFLOAT3 camera_look;
	DirectX::XMFLOAT3 camera_up;   
	DirectX::XMFLOAT3 camera_position;
public:
	SimpleCamera();

	void RotationRight(float angle);

	void RotationUp(float angle);

	void RotationLook(float angle);

	void RotationX(float angle);

	void RotationY(float angle);

	void RotationZ(float angle);

	void WalkFront(float distance);

	void WalkRight(float distance);

	void WalkUp(float distance);

	void CountViewMatrix(DirectX::XMFLOAT4X4* view_matrix);

	void CountViewMatrix(DirectX::XMFLOAT3 rec_look, DirectX::XMFLOAT3 rec_up, DirectX::XMFLOAT3 rec_pos, DirectX::XMFLOAT4X4* matrix);

	void CountInvviewMatrix(DirectX::XMFLOAT4X4* inv_view_matrix);

	void CountInvviewMatrix(DirectX::XMFLOAT3 rec_look, DirectX::XMFLOAT3 rec_up, DirectX::XMFLOAT3 rec_pos, DirectX::XMFLOAT4X4* inv_view_matrix);

	void GetViewPosition(DirectX::XMFLOAT4* view_pos);

	void GetViewDirect(DirectX::XMFLOAT3* view_direct);

	void GetRightDirect(DirectX::XMFLOAT3* right_direct);

	void SetCamera(DirectX::XMFLOAT3 rec_look, DirectX::XMFLOAT3 rec_up, DirectX::XMFLOAT3 rec_pos);

	void ResetCamera();
};