#include "Scene.h"

#include "Camera.h"
#include "shaderClass.h"

void Scene::Init()
{
	shader = new Shader("shaders/default.vert", "shaders/default.frag");
}

void Scene::DrawOpaque(const FrameContext& frame, Camera& camera)
{
	glDepthMask(GL_TRUE);
	shader->Activate();
	camera.Matrix(45.0f, frame.globeRadius * 0.01f, frame.globeRadius * 50.0f, *shader, "camMatrix");

	// Opaque interior and base will be drawn here.
}
