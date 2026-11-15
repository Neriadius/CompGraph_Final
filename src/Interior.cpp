#include "Interior.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "InteriorLayout.h"
#include "shaderClass.h"

namespace
{
	const float WINDOW_FADE_SECONDS = 0.8f;

	// Light colours (colour * intensity) and falloff, tuned for a globe of radius 1
	const glm::vec3 FIRE_COLOR(1.00f, 0.50f, 0.18f);
	const float FIRE_INTENSITY = 2.6f, FIRE_SOFT = 0.15f, FIRE_RANGE = 0.85f;
	const glm::vec3 WINDOW_COLOR(1.00f, 0.66f, 0.32f);
	const float WINDOW_INTENSITY = 1.3f, WINDOW_SOFT = 0.08f, WINDOW_RANGE = 0.36f;
	const float LANTERN_INTENSITY = 0.9f, LANTERN_SOFT = 0.06f, LANTERN_RANGE = 0.26f;

	// Emission colours of glowing surfaces (windows, coals)
	const glm::vec3 WINDOW_EMIT(1.00f, 0.74f, 0.40f);
	const glm::vec3 FIRE_EMIT(1.00f, 0.38f, 0.08f);
	const float COAL_SMOULDER = 0.12f;   // coals still glow a little when the fire is out

	// A program that failed to link must not be used: glUseProgram would fail and the
	// previous program (e.g. the brass key) would draw our meshes instead.
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

	MeshBuilder mb;
	BuildGround(mb);
	BuildTrees(mb);
	BuildHut(mb, anchors);
	BuildFirePit(mb, anchors);
	BuildSeats(mb);
	staticMesh.Upload(mb);
	std::cout << "Interior: " << mb.VertexCount() << " vertices, " << mb.indices.size() / 3 << " triangles" << std::endl;

	character.Init();
	campfire.Init();
	smoke.Init();

	using namespace layout;
	fireLocal = glm::translate(glm::mat4(1.0f), anchors.fire);
	characterLocal = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(CHAR_X, GroundY(CHAR_X, CHAR_Z), CHAR_Z)),
		CHAR_YAW, glm::vec3(0, 1, 0));
}

bool Interior::Inputs(unsigned char key)
{
	switch (key)
	{
	case 'l': case 'L': windowTarget = windowTarget > 0.5f ? 0.0f : 1.0f; return true;
	case 'f': case 'F': campfire.Toggle(); return true;
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
		w.color = WINDOW_COLOR * WINDOW_INTENSITY * windowLevel * candle;
		w.softRadius = WINDOW_SOFT;
		w.range = WINDOW_RANGE;
	}

	PointLight& lantern = lights.points[n++];
	lantern.position = world(anchors.lantern);
	lantern.color = WINDOW_COLOR * LANTERN_INTENSITY * windowLevel;
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

void Interior::Draw(const FrameContext& frame, Camera& camera)
{
	if (!shaderOk) return;
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	shader->Activate();
	camera.Matrix(45.0f, frame.globeRadius * 0.01f, frame.globeRadius * 50.0f, *shader, "camMatrix");
	glUniform3fv(glGetUniformLocation(shader->ID, "camPos"), 1, glm::value_ptr(frame.camPos));
	glUniform1f(glGetUniformLocation(shader->ID, "time"), frame.time);
	UploadSceneLights(shader->ID, lights);
	glUniform1f(glGetUniformLocation(shader->ID, "windowGlow"), windowLevel);
	glUniform1f(glGetUniformLocation(shader->ID, "fireGlow"), COAL_SMOULDER + campfire.Brightness());
	glUniform3fv(glGetUniformLocation(shader->ID, "windowEmitColor"), 1, glm::value_ptr(WINDOW_EMIT));
	glUniform3fv(glGetUniformLocation(shader->ID, "fireEmitColor"), 1, glm::value_ptr(FIRE_EMIT));

	SetModel(frame.globeModel);
	staticMesh.Draw();

	character.Draw(frame.globeModel * characterLocal, frame.time,
		[this](const glm::mat4& m) { SetModel(m); });

	const glm::mat4 fireModel = frame.globeModel * fireLocal;
	campfire.DrawFlame(fireModel, camera, frame.time);

	// transparent effects last (no depth writes): fire glow + sparks, chimney smoke
	campfire.DrawGlow(fireModel, camera, frame.time);
	smoke.Draw(glm::vec3(frame.globeModel * glm::vec4(anchors.chimneyTop, 1.0f)), windowLevel, camera, frame.time);
}
