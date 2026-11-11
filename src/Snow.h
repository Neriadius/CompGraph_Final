#ifndef SNOW_H
#define SNOW_H

#include "FrameContext.h"

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
	Shader* shader = nullptr;
};

#endif
