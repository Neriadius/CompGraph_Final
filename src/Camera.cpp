#include "Camera.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>

glm::vec3 CylinderPoint::At(const glm::vec3& center) const
{
	float a = glm::radians(angle);
	return center + glm::vec3(radius * std::cos(a), height, radius * std::sin(a));
}

Camera::Camera(int width, int height, glm::vec3 target, float sceneRadius)
	: width(width), height(height), target(target), sceneRadius(sceneRadius)
{
	orbit.angle  = 90.0f;                 // start in front of the globe (+Z side)
	orbit.radius = sceneRadius * 3.0f;
	orbit.height = sceneRadius * 0.4f;    // slightly above, looking down at the scene

	nearPlane = sceneRadius * 0.01f;
	farPlane  = sceneRadius * 50.0f;

	UpdatePosition();
}

// ---------------- ORBIT ----------------
void Camera::UpdatePosition()
{
	Position = orbit.At(target);
	Orientation = glm::normalize(target - Position);
}

void Camera::Rotate(float dAngleDeg)
{
	orbit.angle += dAngleDeg;
	if (orbit.angle >= 360.0f) orbit.angle -= 360.0f;
	if (orbit.angle < 0.0f)    orbit.angle += 360.0f;
	UpdatePosition();
}

void Camera::Raise(float dHeight)
{
	orbit.height = std::clamp(orbit.height + dHeight, sceneRadius * minHeightK, sceneRadius * maxHeightK);
	UpdatePosition();
}

void Camera::Zoom(float dRadius)
{
	orbit.radius = std::clamp(orbit.radius + dRadius, sceneRadius * minRadiusK, sceneRadius * maxRadiusK);
	UpdatePosition();
}

// ---------------- FREE ----------------
void Camera::UpdateOrientation()
{
	pitch = std::clamp(pitch, -89.0f, 89.0f);   // never parallel to Up -> no flip
	float y = glm::radians(yaw), p = glm::radians(pitch);
	Orientation = glm::normalize(glm::vec3(std::cos(y) * std::cos(p), std::sin(p), std::sin(y) * std::cos(p)));
}

void Camera::Move(const glm::vec3& dir, float amount)
{
	Position += dir * amount;
}

void Camera::MouseLook(int dx, int dy)
{
	if (mode != FREE) return;
	yaw   += dx * mouseSensitivity;
	pitch -= dy * mouseSensitivity;             // screen Y goes down
	UpdateOrientation();
}

// Switch mode without a jump: FREE starts from the current view,
// ORBIT returns to the cylinder around the globe.
void Camera::ToggleMode()
{
	if (mode == ORBIT)
	{
		mode = FREE;
		yaw = glm::degrees(std::atan2(Orientation.z, Orientation.x));
		pitch = glm::degrees(std::asin(std::clamp(Orientation.y, -1.0f, 1.0f)));
		UpdateOrientation();
	}
	else
	{
		mode = ORBIT;
		UpdatePosition();
	}
}

// ---------------- matrices ----------------
glm::mat4 Camera::ViewMatrix() const
{
	return glm::lookAt(Position, Position + Orientation, Up);
}

glm::mat4 Camera::ProjectionMatrix() const
{
	float aspect = (float)width / (float)(height > 0 ? height : 1);

	if (perspective)
		return glm::perspective(glm::radians(fovDeg), aspect, nearPlane, farPlane);

	float h = sceneRadius * 1.2f;
	float w = h * aspect;
	return glm::ortho(-w, w, -h, h, nearPlane, farPlane);
}

void Camera::Matrix(float FOVdeg, float nearPl, float farPl, Shader& shader, const char* uniform)
{
	fovDeg = FOVdeg;
	nearPlane = nearPl;
	farPlane = farPl;

	glm::mat4 m = ProjectionMatrix() * ViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, uniform), 1, GL_FALSE, glm::value_ptr(m));
}

// ---------------- input ----------------
bool Camera::Inputs(unsigned char key)
{
	const float step = sceneRadius * 0.05f;

	switch (key)
	{
	case 'c': case 'C': ToggleMode(); return true;
	case 'p': case 'P': perspective = !perspective; return true;
	}

	if (mode == ORBIT)
	{
		switch (key)
		{
		case '+': case '=': Zoom(-step); return true;
		case '-': case '_': Zoom(step);  return true;
		}
		return false;
	}

	// FREE
	const float s = step * 2.0f * moveSpeed;
	glm::vec3 right = glm::normalize(glm::cross(Orientation, Up));

	switch (key)
	{
	case 'w': case 'W': Move(Orientation,  s); return true;
	case 's': case 'S': Move(Orientation, -s); return true;
	case 'd': case 'D': Move(right,        s); return true;
	case 'a': case 'A': Move(right,       -s); return true;
	case 'e': case 'E': Move(Up,           s); return true;
	case 'q': case 'Q': Move(Up,          -s); return true;
	case '+': case '=': moveSpeed = std::min(moveSpeed * 1.25f, 20.0f); return true;
	case '-': case '_': moveSpeed = std::max(moveSpeed * 0.8f, 0.05f); return true;
	}
	return false;
}

bool Camera::SpecialInputs(int key)
{
	if (mode == ORBIT)
	{
		const float angleStep = 3.0f;
		const float heightStep = sceneRadius * 0.05f;

		switch (key)
		{
		case GLUT_KEY_LEFT:  Rotate(-angleStep); return true;
		case GLUT_KEY_RIGHT: Rotate(angleStep);  return true;
		case GLUT_KEY_UP:    Raise(heightStep);  return true;
		case GLUT_KEY_DOWN:  Raise(-heightStep); return true;
		}
		return false;
	}

	// FREE: arrows turn the view (keyboard-only alternative to the mouse)
	const float turn = 3.0f;
	switch (key)
	{
	case GLUT_KEY_LEFT:  yaw -= turn;   break;
	case GLUT_KEY_RIGHT: yaw += turn;   break;
	case GLUT_KEY_UP:    pitch += turn; break;
	case GLUT_KEY_DOWN:  pitch -= turn; break;
	default: return false;
	}
	UpdateOrientation();
	return true;
}

void Camera::Resize(int newWidth, int newHeight)
{
	width = newWidth;
	height = newHeight;
}
