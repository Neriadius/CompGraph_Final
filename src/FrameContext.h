#ifndef FRAME_CONTEXT_H
#define FRAME_CONTEXT_H

#include <glm/glm.hpp>

struct FrameContext
{
	float dt;
	float time;
	float shakeIntensity;
	glm::vec3 camPos;
	bool cameraInside;
	glm::vec3 globeCenter;
	float globeRadius;
	// Model matrix with the shake jitter applied. All globe contents must use it.
	glm::mat4 globeModel;
};

#endif
