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
	// Day / night (I key, Person 3): 0 = night, 1 = day, and the key light (moon / sun)
	float daylight = 0.0f;
	glm::vec3 keyLightDir = glm::normalize(glm::vec3(-0.4f, 0.8f, 0.5f));
};

#endif
