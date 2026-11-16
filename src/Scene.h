#ifndef SCENE_H
#define SCENE_H

#include <glad/glad.h>

#include "FrameContext.h"
#include "SceneLights.h"

class Camera;
class Interior;
class Shader;
class Texture;
class VAO;
class WindingKey;
class Nameplate;

class Scene
{
public:
	void Init();
	void Update(const FrameContext& frame);            // once per frame, before drawing
	void DrawOpaque(const FrameContext& frame, Camera& camera);
	bool Inputs(unsigned char key);                    // L = hut lights, F = campfire

	// Interior lights (fire, windows, lantern, moon) in world space for any shader (Person 3 -> 4)
	const SceneLights& Lights() const;

private:
	Shader* shader = nullptr;

	// Wooden base (Person 4)
	Shader* baseShader = nullptr;
	Texture* woodTexture = nullptr;
	VAO* baseVAO = nullptr;
	GLsizei baseIndexCount = 0;

	// Winding key on the side of the base (Person 4)
	WindingKey* windingKey = nullptr;

	// Metal title plate on the front of the wooden base
	Nameplate* nameplate = nullptr;

	// Ground, trees, hut, campfire, character (Person 3)
	Interior* interior = nullptr;
};

#endif
