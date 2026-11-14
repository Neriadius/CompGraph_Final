#ifndef SNOW_H
#define SNOW_H

#include "FrameContext.h"
#include <glad/glad.h>
#include <random>
#include <vector>

class Camera;
class Shader;

// Owner: Person 2. Implement snow simulation and transparent flake rendering here.
// Snow sets and restores its own GL state:
// blending ON, depth test ON, depth mask OFF while drawing.
// Keep flakes inside globeRadius and handle cameraInside without huge or vanishing close flakes.
// Restore glDepthMask(GL_TRUE) and disable blending before returning.
class Snow
{
public:
	void Init();
	void Update(const FrameContext& frame);
	void Draw(const FrameContext& frame, Camera& camera);

private:
	struct Particle
	{
		glm::vec3 pos, vel;
		float size;
		float phase;
		float fallSpeed;
		bool resting;
	};

	struct SnowVertex
	{
		float x, y, z, size, phase;
	};

	Shader* shader = nullptr;
	std::vector<Particle> particles;
	std::vector<SnowVertex> vertices;
	GLuint vao = 0, vbo = 0;
	bool radiusInitialized = false;
	float agitation = 0.0f;
	std::mt19937 rng;
};

#endif
