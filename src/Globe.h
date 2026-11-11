#ifndef GLOBE_H
#define GLOBE_H

#include <glad/glad.h>

#include "FrameContext.h"

class Camera;
class EBO;
class Shader;
class VAO;
class VBO;

enum class GlassPass { Back, Front };

// Owner: Person 1. Implement the glass sphere passes here.
// Back will render back faces and Front will render front faces.
class Globe
{
public:
	void Init(float radius);
	void DrawGlass(GlassPass pass, const FrameContext& frame, Camera& camera);

private:
	Shader* shader = nullptr;
	VAO* sphereVAO = nullptr;
	VBO* sphereVBO = nullptr;
	EBO* sphereEBO = nullptr;
	GLsizei sphereIndexCount = 0;
	GLuint skyCubemap = 0;
	GLint skyBoxLocation = -1;
	GLint reflectStrengthLocation = -1;
};

#endif
