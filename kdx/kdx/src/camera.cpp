#include "camera.h"
#include "constants.h"

#include <D3D11.h>
#include <D3DX10math.h>

Camera::Camera() : rot_(), pos_(), fovy_(0), aspect_(0), zNear_(0), zFar_(0)
{

}

Camera::Camera(float fovy, float aspect, float zNear, float zFar)
{
	init(fovy, aspect, zNear, zFar);
}

Camera::~Camera()
{

}

void Camera::init(float fovy, float aspect, float zNear, float zFar)
{
	fovy_ = fovy; aspect_ = aspect; zNear_ = zNear; zFar_ = zFar;
}

void Camera::toMatrixProj(D3DXMATRIX &matrix) const
{
	//D3DXMatrixPerspectiveFovRH(&matrix, fovy_, aspect_, zNear_, zFar_);
	D3DXMatrixPerspectiveFovLH(&matrix, fovy_, aspect_, zNear_, zFar_);
}

Camera& Camera::rotate(fl2 &torot)
{
	rot_ += torot;
    // limit pitch to -90 to 90
    rot_.x = min(90, max(-90, rot_.x));
    // limit left/right to -180 to 180 to prevent overflow
    if (rot_.y > 180) {
        rot_.y = rot_.y - 360;
    } else if (rot_.y < -180) {
        rot_.y = rot_.y + 360;
    }
    return *this;
}

Camera& Camera::setRot(fl2 &newrot)
{
	rot_ = newrot;
	return *this;
}

Camera& Camera::setPos(fl3 &newpos)
{
	pos_ = newpos;
	return *this;
}

fl3 Camera::getUp() const
{
	D3DXVECTOR3 up;
	up.y = 1.0f;

	D3DXMATRIX rotmatrix;
	float pitch = DEGTORAD(rot_.x);
	float yaw = DEGTORAD(rot_.y);
	D3DXMatrixRotationYawPitchRoll(&rotmatrix, yaw, pitch, 0);

	D3DXVec3TransformCoord(&up, &up, &rotmatrix);
	return up;
}

FirstPersonCamera::FirstPersonCamera() : Camera() {}

FirstPersonCamera::FirstPersonCamera(float fovy, float aspect, float zNear, float zFar) :
Camera(fovy, aspect, zNear, zFar) {}

FirstPersonCamera::~FirstPersonCamera() {}

Camera& FirstPersonCamera::move(fl3 &tomove)
{
	float r = tomove.z * cos(DEGTORAD(rot_.x));
	// in LH coordinates, positive Z is moving forward, so r is positive
	// to switch from RH, just negate rot_.y for sins
    pos_.x += r * sin(DEGTORAD(-rot_.y)) + tomove.x * cos(DEGTORAD(rot_.y));
    pos_.z += r * cos(DEGTORAD(rot_.y)) - tomove.x * sin(DEGTORAD(-rot_.y));
    pos_.y += tomove.y + tomove.z * sin(DEGTORAD(rot_.x)); // becomes a + in LH coordinates
    return *this;
}

void FirstPersonCamera::toMatrixView(D3DXMATRIX &matrix) const
{
	// for RH coordinate system we want pitch/yaw to be negative camera rotation angles
	// for LH, we want positive
	float pitch = DEGTORAD(rot_.x);
	float yaw = DEGTORAD(rot_.y);
	D3DXMATRIX rotationMatrix;
	D3DXMATRIX translationMatrix;

	D3DXMatrixRotationYawPitchRoll(&rotationMatrix, yaw, pitch, 0);
	D3DXMatrixTranslation(&translationMatrix, -pos_.x, -pos_.y, -pos_.z);

	D3DXMatrixMultiply(&matrix, &translationMatrix, &rotationMatrix);
}