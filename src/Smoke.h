#ifndef SMOKE_H
#define SMOKE_H

// Smoke rising from the hut's chimney. Camera-facing puffs generated in shaders/smoke.vert from gl_VertexID

#include <glad/glad.h>
#include <glm/glm.hpp>

class Camera;
class Shader;

class Smoke
{
public:
	void Init();
	// origin = chimney top in world space, amount = 0..1, daylight = 0 night .. 1 day
	void Draw(const glm::vec3& origin, float amount, float daylight, Camera& camera, float time);

private:
	Shader* shader = nullptr;
	GLuint emptyVAO = 0;
	bool shaderOk = false;
};

#endif
