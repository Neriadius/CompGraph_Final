#ifndef SCENE_H
#define SCENE_H

#include "FrameContext.h"

class Camera;
class Shader;

// Owner: Person 3 + 4. Implement opaque interior and base rendering here.
// DrawOpaque keeps depth writes ON.
class Scene
{
public:
	void Init();
	void DrawOpaque(const FrameContext& frame, Camera& camera);

private:
	Shader* shader = nullptr;
};

#endif
