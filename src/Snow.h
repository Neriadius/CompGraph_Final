#ifndef SNOW_H
#define SNOW_H

#include "FrameContext.h"

class Camera;
class Shader;

// Owner: Person 2. Implement snow simulation and transparent flake rendering here.
// Draw contract: depth test ON, depth mask OFF, blending ON; keep flakes inside
// globeRadius and handle cameraInside without huge or vanishing close flakes.
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
