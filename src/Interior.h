#ifndef INTERIOR_H
#define INTERIOR_H

// The tiny world inside the globe (Person 3): snow ground, pine trees, log hut,
// campfire and a character sitting by it. Owns its shaders and the light rig.
//
//   Update(frame)   once per frame: fades, flicker, light positions (world space)
//   Draw(frame)     opaque geometry, then the additive glow and sparks
//   Inputs(key)     L = hut lights on/off, F = campfire on/off
//   Lights()        all interior lights for other shaders (see SceneLights.h)

#include <glm/glm.hpp>

#include "Campfire.h"
#include "Character.h"
#include "FrameContext.h"
#include "InteriorModels.h"
#include "MeshBuilder.h"
#include "SceneLights.h"
#include "Smoke.h"

class Camera;
class Shader;

class Interior
{
public:
	void Init();
	void Update(const FrameContext& frame);
	void Draw(const FrameContext& frame, Camera& camera);
	bool Inputs(unsigned char key);

	const SceneLights& Lights() const { return lights; }

private:
	void SetModel(const glm::mat4& model);

	Shader* shader = nullptr;
	bool shaderOk = false;
	GpuMesh staticMesh;
	Character character;
	Campfire campfire;
	Smoke smoke;
	InteriorAnchors anchors;
	SceneLights lights;

	glm::mat4 fireLocal = glm::mat4(1.0f);       // fire space -> globe space
	glm::mat4 characterLocal = glm::mat4(1.0f);  // character space -> globe space
	glm::mat4 globeModel = glm::mat4(1.0f);      // latest shake matrix

	float windowLevel = 1.0f, windowTarget = 1.0f;
};

#endif
