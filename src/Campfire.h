#ifndef CAMPFIRE_H
#define CAMPFIRE_H

// Animated part of the campfire (Person 3): flame tongues, rising sparks and a soft glow.
// The logs, stones and coals are static geometry (InteriorModels.cpp).
//
// Flicker: one function of time drives the flame stretch, the coal glow and the fire's
// point light, so light and flame always pulse together.

#include <glad/glad.h>
#include <glm/glm.hpp>

class Camera;
class Shader;

class Campfire
{
public:
	void Init();
	void Update(float time, float dt);
	void Toggle() { target = target > 0.5f ? 0.0f : 1.0f; }

	float Level() const { return level; }              // 0 = out, 1 = burning (smooth)
	float Brightness() const { return level * flicker; } // what lights should use
	glm::vec3 LightJitter() const { return jitter; }   // small movement of the light (fire space)

	// Opaque flame (writes depth). fireModel = fire space -> world.
	void DrawFlame(const glm::mat4& fireModel, Camera& camera, float time);
	// Additive glow + sparks (no depth writes). Call after all opaque geometry.
	void DrawGlow(const glm::mat4& fireModel, Camera& camera, float time);

private:
	Shader* flameShader = nullptr;
	Shader* emberShader = nullptr;
	Shader* glowShader = nullptr;
	GLuint flameVAO = 0;
	GLsizei flameCount = 0;
	GLuint emptyVAO = 0;              // core profile needs a VAO even for buffer-less draws
	bool flameOk = false, glowOk = false;

	float level = 1.0f, target = 1.0f;
	float flicker = 1.0f;
	glm::vec3 jitter = glm::vec3(0.0f);
};

#endif
