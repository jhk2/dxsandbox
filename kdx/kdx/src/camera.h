#ifndef CAMERA_H
#define CAMERA_H

#include "utils.h"

struct D3DXMATRIX;

class Camera {
public:
	Camera();
	Camera(float fovy, float aspect, float zNear, float zFar);
	virtual ~Camera();
	void init(float fovy, float aspect, float zNear, float zFar);

	// sending camera parameters to matrices
	void toMatrixProj(D3DXMATRIX &matrix) const;
	virtual void toMatrixView(D3DXMATRIX &matrix) const = 0;

	// means different things in first vs third person
	virtual Camera& move(fl3 &tomove) = 0;
	// rotate camera by degrees
	Camera &rotate(fl2 &torot);
	// set position, rotation
	Camera &setRot(fl2 &newrot);
	Camera &setPos(fl3 &newpos);

	// getters
	fl3 getPos() const { return pos_; }
	fl3 getUp() const;
	fl3 getLook() const;
	
protected:
	fl2 rot_; // rotation in degrees
	fl3 pos_;
	float fovy_, aspect_, zNear_, zFar_;
};

class FirstPersonCamera : public Camera {
public:
	FirstPersonCamera();
	FirstPersonCamera(float fovy, float aspect, float zNear, float zFar);
	virtual ~FirstPersonCamera();
	void toMatrixView(D3DXMATRIX &matrix) const;
	Camera& move(fl3 &tomove);
};
#endif // CAMERA_H