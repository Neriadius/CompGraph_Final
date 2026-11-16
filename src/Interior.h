#ifndef INTERIOR_H
#define INTERIOR_H

// The tiny world inside the globe (Person 3): snow ground, campfire, props (procedural)
// and the OBJ models: log hut, pine trees, bushes, snowman, Typhoeus sitting by the fire.
// Owns its shaders and the light rig.
//
// Placement hierarchy (each model matrix = parent * local):
//   globe (shake) -> hut         -> lantern, chimney smoke, window light
//                 -> seat log     -> Typhoeus (animated by morphing between two poses)
//                 -> fire pit     -> flame, sparks, glow, fire light
//                 -> trees, bushes, snowman
//
//   Update(frame)   once per frame: fades, flicker, light positions (world space)
//   Draw(frame)     opaque geometry, then the additive glow and sparks
//   Inputs(key)     L = hut lights on/off, F = campfire on/off, I = day / night
//   Lights()        all interior lights for other shaders (see SceneLights.h)

#include <glm/glm.hpp>

#include "Campfire.h"
#include "FrameContext.h"
#include "InteriorModels.h"
#include "MeshBuilder.h"
#include "Model.h"
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
	void SetFrameUniforms(GLuint program, const FrameContext& frame, Camera& camera);
	void DrawModels(const FrameContext& frame, Camera& camera);

	Shader* shader = nullptr;          // procedural mesh
	Shader* modelShader = nullptr;     // OBJ models (same fragment shader)
	bool shaderOk = false, modelShaderOk = false;
	GpuMesh staticMesh;

	Model hut, snowman, typhoeus, bush;
	Model trees[3];                    // layout::PINE, PINE_TALL, SHRUB
	Campfire campfire;
	Smoke smoke;
	InteriorAnchors anchors;
	SceneLights lights;

	glm::mat4 fireLocal = glm::mat4(1.0f);       // fire space -> globe space
	glm::mat4 seatLocal = glm::mat4(1.0f);       // seat log (Typhoeus) -> globe space
	glm::mat4 globeModel = glm::mat4(1.0f);      // latest shake matrix

	float windowLevel = 1.0f, windowTarget = 1.0f;
	float dayLevel = 0.0f, dayTarget = 0.0f;        // 0 = night, 1 = day
};

#endif
