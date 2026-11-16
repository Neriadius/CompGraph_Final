#include "Interior.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "InteriorLayout.h"
#include "SceneConstants.h"
#include "shaderClass.h"

namespace
{
	const float WINDOW_FADE_SECONDS = 0.8f;
	const float DAY_FADE_SECONDS = 3.0f;           // sunrise / sunset

	// Light colours (colour * intensity) and falloff, tuned for a globe of radius 1
	const glm::vec3 FIRE_COLOR(1.00f, 0.50f, 0.18f);
	const float FIRE_INTENSITY = 2.6f, FIRE_SOFT = 0.15f, FIRE_RANGE = 0.85f;
	const glm::vec3 WINDOW_COLOR(1.00f, 0.66f, 0.32f);
	const float WINDOW_INTENSITY = 1.7f, WINDOW_SOFT = 0.09f, WINDOW_RANGE = 0.40f;
	const float LANTERN_INTENSITY = 0.9f, LANTERN_SOFT = 0.06f, LANTERN_RANGE = 0.26f;

	// Emission colours of glowing surfaces (windows, coals)
	const glm::vec3 WINDOW_EMIT(1.00f, 0.74f, 0.40f);
	const glm::vec3 FIRE_EMIT(1.00f, 0.38f, 0.08f);
	const float COAL_SMOULDER = 0.12f;   // coals still glow a little when the fire is out

	bool Linked(const Shader* s, const char* name)
	{
		GLint ok = GL_FALSE;
		glGetProgramiv(s->ID, GL_LINK_STATUS, &ok);
		if (!ok) std::cerr << name << ": shader failed (see the error above), not drawn" << std::endl;
		return ok == GL_TRUE;
	}
}

void Interior::Init()
{
	shader = new Shader("shaders/interior.vert", "shaders/interior.frag");
	shaderOk = Linked(shader, "Interior");
	modelShader = new Shader("shaders/model.vert", "shaders/interior.frag");
	modelShaderOk = Linked(modelShader, "Interior models");

	MeshBuilder mb;
	BuildGround(mb);
	BuildHutProps(mb, anchors);
	BuildFirePit(mb, anchors);
	BuildSeats(mb);
	staticMesh.Upload(mb);
	std::cout << "Interior: " << mb.VertexCount() << " vertices, " << mb.indices.size() / 3 << " triangles" << std::endl;

	hut.Load("models/hut/hut.obj");
	trees[layout::PINE].Load("models/trees/pine.obj");
	trees[layout::PINE_TALL].Load("models/trees/pine_tall.obj");
	trees[layout::SHRUB].Load("models/trees/shrub.obj");
	bush.Load("models/trees/bush.obj");
	snowman.Load("models/snowman/snowman.obj");
	typhoeus.Load("models/typhoeus/typhoeus_pose0.obj");
	typhoeus.LoadMorph("models/typhoeus/typhoeus_pose1.obj");

	campfire.Init();
	smoke.Init();

	using namespace layout;
	fireLocal = glm::translate(glm::mat4(1.0f), anchors.fire);
	seatLocal = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(CHAR_X, GroundY(CHAR_X, CHAR_Z), CHAR_Z)),
		CHAR_YAW, glm::vec3(0, 1, 0));
}

bool Interior::Inputs(unsigned char key)
{
	switch (key)
	{
	case 'l': case 'L': windowTarget = windowTarget > 0.5f ? 0.0f : 1.0f; return true;
	case 'f': case 'F': campfire.Toggle(); return true;
	case 'i': case 'I': dayTarget = dayTarget > 0.5f ? 0.0f : 1.0f; return true;
	}
	return false;
}

void Interior::Update(const FrameContext& frame)
{
	globeModel = frame.globeModel;
	campfire.Update(frame.time, frame.dt);

	float step = frame.dt / WINDOW_FADE_SECONDS;
	windowLevel = windowTarget > windowLevel ? std::min(windowTarget, windowLevel + step)
	                                         : std::max(windowTarget, windowLevel - step);

	// day / night: moon <-> sun, ambient, background (eased so it starts and ends softly)
	float dayStep = frame.dt / DAY_FADE_SECONDS;
	dayLevel = dayTarget > dayLevel ? std::min(dayTarget, dayLevel + dayStep) : std::max(dayTarget, dayLevel - dayStep);
	const float day = dayLevel * dayLevel * (3.0f - 2.0f * dayLevel);
	ApplyDaylight(lights, day);
	const float lampScale = 1.0f - 0.5f * day;      // lamps matter less in daylight

	// ---- light rig in world space (moves with the shaking globe) ----
	auto world = [&](const glm::vec3& p) { return glm::vec3(globeModel * glm::vec4(p, 1.0f)); };
	int n = 0;

	PointLight& fire = lights.points[n++];
	fire.position = world(anchors.fire + glm::vec3(0.0f, 0.06f, 0.0f) + campfire.LightJitter());
	fire.color = FIRE_COLOR * FIRE_INTENSITY * campfire.Brightness();
	fire.softRadius = FIRE_SOFT;
	fire.range = FIRE_RANGE;

	for (size_t i = 0; i < anchors.windowLights.size() && n < MAX_POINT_LIGHTS - 1; i++)
	{
		PointLight& w = lights.points[n++];
		float candle = 0.95f + 0.05f * std::sin(frame.time * 3.1f + 1.7f * i);     // gentle lamp flicker
		w.position = world(anchors.windowLights[i]);
		w.color = WINDOW_COLOR * WINDOW_INTENSITY * windowLevel * candle * lampScale;
		w.softRadius = WINDOW_SOFT;
		w.range = WINDOW_RANGE;
	}

	PointLight& lantern = lights.points[n++];
	lantern.position = world(anchors.lantern);
	lantern.color = WINDOW_COLOR * LANTERN_INTENSITY * windowLevel * lampScale;
	lantern.softRadius = LANTERN_SOFT;
	lantern.range = LANTERN_RANGE;

	lights.pointCount = n;
}

void Interior::SetModel(const glm::mat4& model)
{
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix3fv(glGetUniformLocation(shader->ID, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
}

void Interior::SetFrameUniforms(GLuint program, const FrameContext& frame, Camera& camera)
{
	glm::mat4 camMatrix = camera.ProjectionMatrix() * camera.ViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(program, "camMatrix"), 1, GL_FALSE, glm::value_ptr(camMatrix));
	glUniform3fv(glGetUniformLocation(program, "camPos"), 1, glm::value_ptr(frame.camPos));
	glUniform1f(glGetUniformLocation(program, "time"), frame.time);
	UploadSceneLights(program, lights);
	glUniform1f(glGetUniformLocation(program, "windowGlow"), windowLevel * (1.0f - 0.4f * lights.daylight));
	glUniform1f(glGetUniformLocation(program, "fireGlow"), COAL_SMOULDER + campfire.Brightness());
	glUniform3fv(glGetUniformLocation(program, "windowEmitColor"), 1, glm::value_ptr(WINDOW_EMIT));
	glUniform3fv(glGetUniformLocation(program, "fireEmitColor"), 1, glm::value_ptr(FIRE_EMIT));
}

// Model matrix of an object standing on the snow: globe * translate * rotate * scale.
// On a slope it is sunk a little deeper so the downhill side of the trunk does not float.
static glm::mat4 OnGround(const glm::mat4& globe, float x, float z, float yaw, float scale, float sink = 0.0f)
{
	const float e = 0.01f;
	float slope = std::hypot(layout::TerrainOffset(x + e, z) - layout::TerrainOffset(x - e, z),
		layout::TerrainOffset(x, z + e) - layout::TerrainOffset(x, z - e)) / (2.0f * e);
	sink += 0.03f * scale * slope;
	glm::mat4 m = glm::translate(globe, glm::vec3(x, GROUND_Y_RATIO + layout::TerrainOffset(x, z) - sink, z));
	m = glm::rotate(m, yaw, glm::vec3(0, 1, 0));
	return glm::scale(m, glm::vec3(scale));
}

void Interior::DrawModels(const FrameContext& frame, Camera& camera)
{
	using namespace layout;
	const glm::mat4& globe = frame.globeModel;
	modelShader->Activate();
	SetFrameUniforms(modelShader->ID, frame, camera);
	glUniform1i(glGetUniformLocation(modelShader->ID, "twoSided"), 1);   // cards and cloth seen from both sides
	const GLuint p = modelShader->ID;

	hut.Draw(p, globe * HutMatrix());

	for (const TreeSpot& t : TREES)
		trees[t.model].Draw(p, OnGround(globe, t.x, t.z, t.yawDeg * DEG, t.scale, 0.006f));
	for (const BushSpot& b : BUSHES)
		bush.Draw(p, OnGround(globe, b.x, b.z, b.yawDeg * DEG, b.scale, 0.006f));
	snowman.Draw(p, OnGround(globe, SNOWMAN_X, SNOWMAN_Z, SNOWMAN_YAW, 1.0f, 0.012f));

	// Typhoeus sits on the seat log. Animation: she watches the fire, then now and then
	// glances aside and lifts her hands (pose 1), plus a slow breathing sway.
	const float t = frame.time;
	const float cycle = std::fmod(t, 10.0f) / 10.0f;
	auto smooth = [](float a, float b, float x) { float k = std::clamp((x - a) / (b - a), 0.0f, 1.0f); return k * k * (3 - 2 * k); };
	float glance = smooth(0.35f, 0.50f, cycle) * (1.0f - smooth(0.72f, 0.88f, cycle));
	float breath = 0.06f * (0.5f + 0.5f * std::sin(t * 2.1f));
	typhoeus.Draw(p, globe * seatLocal, std::min(1.0f, glance + breath));
}

void Interior::Draw(const FrameContext& frame, Camera& camera)
{
	if (!shaderOk) return;
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	shader->Activate();
	camera.Matrix(45.0f, frame.globeRadius * 0.01f, frame.globeRadius * 50.0f, *shader, "camMatrix");
	SetFrameUniforms(shader->ID, frame, camera);
	SetModel(frame.globeModel);
	staticMesh.Draw();

	if (modelShaderOk) DrawModels(frame, camera);

	const glm::mat4 fireModel = frame.globeModel * fireLocal;
	campfire.DrawFlame(fireModel, camera, frame.time);

	// transparent effects last (no depth writes): fire glow + sparks, chimney smoke
	campfire.DrawGlow(fireModel, camera, frame.time);
	smoke.Draw(glm::vec3(frame.globeModel * glm::vec4(anchors.chimneyTop, 1.0f)), windowLevel, lights.daylight,
		camera, frame.time);
}
