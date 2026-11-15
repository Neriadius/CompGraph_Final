#ifndef SMOKE_H
#define SMOKE_H

// Smoke rising from the hut's chimney (Person 3). Camera-facing puffs generated in
// shaders/smoke.vert from gl_VertexID; this class only sets uniforms and blending.

#include <glad/glad.h>
#include <glm/glm.hpp>

class Camera;
class Shader;

class Smoke
{
public:
	void Init();
	// origin = chimney top in world space, amount = 0..1
	void Draw(const glm::vec3& origin, float amount, Camera& camera, float time);

private:
	Shader* shader = nullptr;
	GLuint emptyVAO = 0;
	bool shaderOk = false;
};

#endif
