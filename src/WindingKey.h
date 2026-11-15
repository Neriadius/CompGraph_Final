#ifndef WINDING_KEY_H
#define WINDING_KEY_H

#include <glad/glad.h>

#include "FrameContext.h"

class Camera;
class Shader;
class VAO;

// Brass music-box winding key sticking out of the side of the wooden base (Person 4).
// It turns while the globe is shaken (speed follows frame.shakeIntensity)
// and shakes together with the globe (frame.globeModel).
class WindingKey
{
public:
	void Init();
	void Draw(const FrameContext& frame, Camera& camera);

private:
	Shader* shader = nullptr;
	VAO* vao = nullptr;
	GLsizei indexCount = 0;
	float angle = 0.0f;          // current turn, radians
};

#endif
