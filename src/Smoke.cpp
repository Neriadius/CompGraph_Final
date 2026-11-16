#include "Smoke.h"

#include <iostream>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "shaderClass.h"

namespace
{
	const int PUFFS = 14;
	const glm::vec3 NIGHT_SMOKE(0.55f, 0.58f, 0.66f), DAY_SMOKE(0.86f, 0.87f, 0.90f);

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

void Smoke::Init()
{
	shader = new Shader("shaders/smoke.vert", "shaders/smoke.frag");
	shaderOk = Linked(shader, "Smoke");
	glGenVertexArrays(1, &emptyVAO);
}

void Smoke::Draw(const glm::vec3& origin, float amount, float daylight, Camera& camera, float time)
{
	if (amount <= 0.01f || !shaderOk) return;

	glm::vec3 right = glm::normalize(glm::cross(camera.Orientation, camera.Up));
	glm::vec3 up = glm::cross(right, camera.Orientation);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	shader->Activate();
	camera.Matrix(camera.fovDeg, camera.nearPlane, camera.farPlane, *shader, "camMatrix");
	glUniform3fv(glGetUniformLocation(shader->ID, "origin"), 1, glm::value_ptr(origin));
	glUniform3fv(glGetUniformLocation(shader->ID, "camRight"), 1, glm::value_ptr(right));
	glUniform3fv(glGetUniformLocation(shader->ID, "camUp"), 1, glm::value_ptr(up));
	glUniform1f(glGetUniformLocation(shader->ID, "time"), time);
	glUniform1f(glGetUniformLocation(shader->ID, "amount"), amount);
	glUniform1i(glGetUniformLocation(shader->ID, "puffCount"), PUFFS);
	glm::vec3 color = glm::mix(NIGHT_SMOKE, DAY_SMOKE, daylight);
	glUniform3fv(glGetUniformLocation(shader->ID, "smokeColor"), 1, glm::value_ptr(color));

	glBindVertexArray(emptyVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6 * PUFFS);
	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}
