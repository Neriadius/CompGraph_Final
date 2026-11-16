#include "Campfire.h"

#include <cmath>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "shaderClass.h"

namespace
{
	const float PI = 3.14159265358979f;
	const int SPARKS = 28;
	const float FADE_SECONDS = 1.2f;         // fire goes out / catches in this time

	struct Tongue { float x, z, radius, height, lean, seed; };

	// Teardrop profile: 0 at the bottom, widest at t = 1/3, sharp tip at t = 1.
	float Profile(float t) { return std::sqrt(t) * (1.0f - t) / 0.3849f; }

	// Vertex: position(3) normal(3) flame(4) = t, seed, tongue axis x, z
	void AddTongue(const Tongue& g, std::vector<GLfloat>& v, std::vector<GLuint>& idx)
	{
		const int rows = 12, slices = 12;
		const GLuint first = (GLuint)(v.size() / 10);
		const glm::vec2 out = glm::length(glm::vec2(g.x, g.z)) > 0.0f ? glm::normalize(glm::vec2(g.x, g.z)) : glm::vec2(0.0f);

		for (int i = 0; i <= rows; i++)
		{
			float t = (float)i / rows;
			float r = g.radius * Profile(t);
			float dr = g.radius * (Profile(std::min(t + 0.01f, 1.0f)) - Profile(std::max(t - 0.01f, 0.0f))) / 0.02f;
			float y = 0.006f + g.height * t;
			glm::vec2 c = glm::vec2(g.x, g.z) + out * g.lean * t * t;          // outer tongues lean out
			for (int j = 0; j <= slices; j++)
			{
				float a = 2.0f * PI * j / slices;
				glm::vec3 n = glm::normalize(glm::vec3(std::cos(a) * g.height, -dr, std::sin(a) * g.height));
				v.insert(v.end(), { c.x + r * std::cos(a), y, c.y + r * std::sin(a), n.x, n.y, n.z, t, g.seed, c.x, c.y });
			}
		}
		for (int i = 0; i < rows; i++)
			for (int j = 0; j < slices; j++)
			{
				GLuint a = first + i * (slices + 1) + j, b = a + slices + 1;
				idx.insert(idx.end(), { a, b, b + 1, a, b + 1, a + 1 });
			}
	}

	float FlickerAt(float t)
	{
		return 0.80f + 0.09f * std::sin(7.3f * t) + 0.06f * std::sin(13.1f * t + 1.3f)
			+ 0.05f * std::sin(23.7f * t + 0.4f) + 0.04f * std::sin(37.9f * t + 2.1f);
	}

	bool Linked(const Shader* s, const char* name)
	{
		GLint ok = GL_FALSE;
		glGetProgramiv(s->ID, GL_LINK_STATUS, &ok);
		if (!ok) std::cerr << name << ": shader failed (see the error above), not drawn" << std::endl;
		return ok == GL_TRUE;
	}

	void SetMat4(Shader& s, const char* name, const glm::mat4& m)
	{
		glUniformMatrix4fv(glGetUniformLocation(s.ID, name), 1, GL_FALSE, glm::value_ptr(m));
	}
}

void Campfire::Init()
{
	flameShader = new Shader("shaders/flame.vert", "shaders/flame.frag");
	emberShader = new Shader("shaders/embers.vert", "shaders/embers.frag");
	glowShader = new Shader("shaders/glow.vert", "shaders/glow.frag");
	flameOk = Linked(flameShader, "Campfire flame");
	glowOk = Linked(emberShader, "Campfire sparks") && Linked(glowShader, "Campfire glow");

	std::vector<GLfloat> v;
	std::vector<GLuint> idx;
	AddTongue({ 0.0f, 0.0f, 0.020f, 0.125f, 0.0f, 0.0f }, v, idx);                 // tall middle tongue
	for (int k = 0; k < 5; k++)
	{
		float a = 2.0f * PI * k / 5 + 0.4f;
		AddTongue({ 0.016f * std::cos(a), 0.016f * std::sin(a), 0.015f, 0.075f + 0.02f * (k % 3) * 0.5f, 0.010f, 1.0f + k }, v, idx);
	}
	for (int k = 0; k < 4; k++)
	{
		float a = 2.0f * PI * k / 4 + 1.1f;
		AddTongue({ 0.028f * std::cos(a), 0.028f * std::sin(a), 0.010f, 0.045f, 0.012f, 7.0f + k }, v, idx);
	}
	flameCount = (GLsizei)idx.size();

	GLuint vbo, ebo;
	glGenVertexArrays(1, &flameVAO);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);
	glBindVertexArray(flameVAO);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(GLfloat), v.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);
	const GLsizei stride = 10 * sizeof(GLfloat);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &emptyVAO);
}

void Campfire::Update(float time, float dt)
{
	float step = dt / FADE_SECONDS;
	if (level < target) level = std::min(target, level + step);
	else                level = std::max(target, level - step);

	flicker = FlickerAt(time);
	jitter = glm::vec3(0.006f * std::sin(time * 5.1f), 0.004f * std::sin(time * 9.7f + 1.0f), 0.006f * std::cos(time * 4.3f));
}

void Campfire::DrawFlame(const glm::mat4& fireModel, Camera& camera, float time)
{
	if (level <= 0.01f || !flameOk) return;
	flameShader->Activate();
	camera.Matrix(camera.fovDeg, camera.nearPlane, camera.farPlane, *flameShader, "camMatrix");
	SetMat4(*flameShader, "model", fireModel);
	glUniform1f(glGetUniformLocation(flameShader->ID, "time"), time);
	glUniform1f(glGetUniformLocation(flameShader->ID, "fireLevel"), level);
	glUniform3fv(glGetUniformLocation(flameShader->ID, "camPos"), 1, glm::value_ptr(camera.Position));
	glm::vec3 origin(fireModel[3]);
	glUniform3fv(glGetUniformLocation(flameShader->ID, "fireOrigin"), 1, glm::value_ptr(origin));

	glBindVertexArray(flameVAO);
	glDrawElements(GL_TRIANGLES, flameCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Campfire::DrawGlow(const glm::mat4& fireModel, Camera& camera, float time)
{
	if (level <= 0.01f || !glowOk) return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDepthMask(GL_FALSE);
	glBindVertexArray(emptyVAO);

	glm::vec3 centre = glm::vec3(fireModel * glm::vec4(0.0f, 0.05f, 0.0f, 1.0f));
	glm::vec3 toCam = camera.Position - centre;
	float dist = glm::length(toCam);
	centre += toCam / std::max(dist, 1e-4f) * std::min(0.07f, 0.5f * dist);
	glm::vec3 right = glm::normalize(glm::cross(camera.Orientation, camera.Up));
	glm::vec3 up = glm::cross(right, camera.Orientation);

	glowShader->Activate();
	camera.Matrix(camera.fovDeg, camera.nearPlane, camera.farPlane, *glowShader, "camMatrix");
	glUniform3fv(glGetUniformLocation(glowShader->ID, "center"), 1, glm::value_ptr(centre));
	glUniform3fv(glGetUniformLocation(glowShader->ID, "camRight"), 1, glm::value_ptr(right));
	glUniform3fv(glGetUniformLocation(glowShader->ID, "camUp"), 1, glm::value_ptr(up));
	glUniform1f(glGetUniformLocation(glowShader->ID, "size"), 0.17f * (0.6f + 0.4f * level));
	glm::vec3 glow = glm::vec3(1.0f, 0.42f, 0.12f) * 0.32f * Brightness();
	glUniform3fv(glGetUniformLocation(glowShader->ID, "glowColor"), 1, glm::value_ptr(glow));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// sparks
	glEnable(GL_PROGRAM_POINT_SIZE);
	emberShader->Activate();
	camera.Matrix(camera.fovDeg, camera.nearPlane, camera.farPlane, *emberShader, "camMatrix");
	SetMat4(*emberShader, "model", fireModel);
	glUniform1f(glGetUniformLocation(emberShader->ID, "time"), time);
	glUniform1f(glGetUniformLocation(emberShader->ID, "fireLevel"), level);
	float pointScale = camera.height / (2.0f * std::tan(glm::radians(camera.fovDeg) * 0.5f));
	glUniform1f(glGetUniformLocation(emberShader->ID, "pointScale"), pointScale);
	glDrawArrays(GL_POINTS, 0, SPARKS);
	glDisable(GL_PROGRAM_POINT_SIZE);

	glBindVertexArray(0);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}
