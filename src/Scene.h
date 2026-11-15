#ifndef SCENE_H
#define SCENE_H

#include <glad/glad.h>

#include "FrameContext.h"

class Camera;
class Shader;
class Texture;
class VAO;
class WindingKey;

// Owner: Person 3 + 4. Implement opaque interior and base rendering here.
// DrawOpaque keeps depth writes ON.
class Scene
{
public:
	void Init();
	void DrawOpaque(const FrameContext& frame, Camera& camera);

private:
	Shader* shader = nullptr;

	// Wooden base (Person 4)
	Shader* baseShader = nullptr;
	Texture* woodTexture = nullptr;
	VAO* baseVAO = nullptr;
	GLsizei baseIndexCount = 0;

	// Winding key on the side of the base (Person 4)
	WindingKey* windingKey = nullptr;
};

#endif
