#include "Snow.h"

#include "Camera.h"
#include "shaderClass.h"

void Snow::Init()
{
	shader = new Shader("shaders/default.vert", "shaders/default.frag");
}

void Snow::Update(const FrameContext&)
{
	// Snow simulation will be updated here.
}

void Snow::Draw(const FrameContext& frame, Camera& camera)
{
	shader->Activate();
	camera.Matrix(45.0f, frame.globeRadius * 0.01f, frame.globeRadius * 50.0f, *shader, "camMatrix");

	// Transparent flakes will be drawn here.
}
