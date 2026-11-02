#ifndef CAMERA_CLASS_H
#define CAMERA_CLASS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shaderClass.h"

// A point in cylindrical coordinates around a center (cylinder axis = Y, Y is "up"):
//   X = R*cos(theta),  Z = R*sin(theta),  Y = H
struct CylinderPoint
{
	float angle;    // theta, degrees
	float radius;   // R
	float height;   // H (relative to the center)

	glm::vec3 At(const glm::vec3& center) const;
};

// Two modes (toggle with C):
//   ORBIT : moves on a cylinder around the globe and always looks at it
//   FREE  : free-fly camera, can go anywhere, also INSIDE the globe
class Camera
{
public:
	enum Mode { ORBIT, FREE };

	glm::vec3 Position;
	glm::vec3 Orientation;                     // unit vector, camera -> where it looks
	glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

	int width;
	int height;

	Mode mode = ORBIT;

	// ORBIT state
	glm::vec3 target;
	CylinderPoint orbit;
	float sceneRadius;
	float minRadiusK = 1.6f, maxRadiusK = 6.0f;     // zoom limits (x sceneRadius)
	float minHeightK = -0.3f, maxHeightK = 2.5f;    // height limits (x sceneRadius)

	// FREE state
	float yaw = -90.0f;                        // degrees, -90 = looking along -Z
	float pitch = 0.0f;                        // degrees, clamped to +-89
	float moveSpeed = 1.0f;                    // multiplier of the base step
	float mouseSensitivity = 0.2f;             // degrees per pixel

	bool perspective = true;                   // true = perspective, false = parallel

	float fovDeg = 45.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;

	Camera(int width, int height, glm::vec3 target, float sceneRadius);

	glm::mat4 ViewMatrix() const;              // glm::lookAt(Position, Position + Orientation, Up)
	glm::mat4 ProjectionMatrix() const;

	// Uploads projection * view to the shader
	void Matrix(float FOVdeg, float nearPlane, float farPlane, Shader& shader, const char* uniform);

	// GLUT input. Return true if the key was used.
	//   both modes : C mode, P projection
	//   ORBIT      : + / - zoom, arrows angle/height
	//   FREE       : W/S forward/back, A/D strafe, Q/E down/up,
	//                arrows turn, + / - speed
	bool Inputs(unsigned char key);
	bool SpecialInputs(int key);
	void MouseLook(int dx, int dy);            // FREE mode: call with mouse movement in pixels

	void Resize(int newWidth, int newHeight);
	void ToggleMode();

private:
	void UpdatePosition();                     // ORBIT: Position/Orientation from cylinder
	void UpdateOrientation();                  // FREE: Orientation from yaw/pitch
	void Rotate(float dAngleDeg);
	void Raise(float dHeight);
	void Zoom(float dRadius);
	void Move(const glm::vec3& dir, float amount);
};

#endif
