#include "WindingKey.h"

#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"

namespace
{
	const float MOUNT_Y = -0.80f;
	const float MOUNT_R = 0.967f;
	const float MOUNT_ANGLE_DEG = 35.0f;      // 0 = towards +Z (the start camera), positive = towards +X

	const float TURN_SPEED = 20.0f;           // radians per second at shake intensity 1

	struct Mesh
	{
		std::vector<GLfloat> vertices;        // position(3) normal(3)
		std::vector<GLuint> indices;

		GLuint Add(const glm::vec3& p, const glm::vec3& n)
		{
			GLuint index = (GLuint)(vertices.size() / 6);
			vertices.insert(vertices.end(), { p.x, p.y, p.z, n.x, n.y, n.z });
			return index;
		}
	};

	// Axis-aligned box
	void AddBox(Mesh& m, glm::vec3 c, glm::vec3 h)
	{
		struct Face { glm::vec3 n, u, v; };    // cross(u, v) = n  ->  counter-clockwise seen from outside
		static const Face faces[6] = {
			{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } },
			{ { -1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 } },
			{ { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 0 } },
			{ { 0, -1, 0 }, { 1, 0, 0 }, { 0, 0, 1 } },
			{ { 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 } },
			{ { 0, 0, -1 }, { 0, 1, 0 }, { 1, 0, 0 } },
		};
		for (const Face& f : faces)
		{
			float hn = glm::dot(glm::abs(f.n), h);
			float hu = glm::dot(glm::abs(f.u), h);
			float hv = glm::dot(glm::abs(f.v), h);
			glm::vec3 centre = c + f.n * hn;
			GLuint a = m.Add(centre - f.u * hu - f.v * hv, f.n);
			GLuint b = m.Add(centre + f.u * hu - f.v * hv, f.n);
			GLuint d = m.Add(centre + f.u * hu + f.v * hv, f.n);
			GLuint e = m.Add(centre - f.u * hu + f.v * hv, f.n);
			m.indices.insert(m.indices.end(), { a, b, d, a, d, e });
		}
	}

	// Cylinder around the Z axis from z0 to z1 (with end caps)
	void AddCylinderZ(Mesh& m, float radius, float z0, float z1, int slices, float cx = 0.0f, float cy = 0.0f)
	{
		const float PI = 3.14159265358979f;

		// side
		for (int j = 0; j < slices; j++)
		{
			float t0 = 2.0f * PI * j / slices, t1 = 2.0f * PI * (j + 1) / slices;
			glm::vec3 n0(std::cos(t0), std::sin(t0), 0), n1(std::cos(t1), std::sin(t1), 0);
			GLuint a = m.Add(glm::vec3(cx + n0.x * radius, cy + n0.y * radius, z0), n0);
			GLuint b = m.Add(glm::vec3(cx + n0.x * radius, cy + n0.y * radius, z1), n0);
			GLuint c = m.Add(glm::vec3(cx + n1.x * radius, cy + n1.y * radius, z0), n1);
			GLuint d = m.Add(glm::vec3(cx + n1.x * radius, cy + n1.y * radius, z1), n1);
			m.indices.insert(m.indices.end(), { a, c, d, a, d, b });
		}
		// caps
		for (int cap = 0; cap < 2; cap++)
		{
			float z = cap ? z1 : z0;
			glm::vec3 n(0, 0, cap ? 1.0f : -1.0f);
			GLuint centre = m.Add(glm::vec3(cx, cy, z), n);
			for (int j = 0; j < slices; j++)
			{
				float t0 = 2.0f * PI * j / slices, t1 = 2.0f * PI * (j + 1) / slices;
				GLuint p0 = m.Add(glm::vec3(cx + std::cos(t0) * radius, cy + std::sin(t0) * radius, z), n);
				GLuint p1 = m.Add(glm::vec3(cx + std::cos(t1) * radius, cy + std::sin(t1) * radius, z), n);
				if (cap) m.indices.insert(m.indices.end(), { centre, p0, p1 });
				else     m.indices.insert(m.indices.end(), { centre, p1, p0 });
			}
		}
	}

	// Key in its own space: Z = outwards (the shaft axis), the key head lies in the XY plane.
	void BuildKey(Mesh& m)
	{
		AddCylinderZ(m, 0.030f, -0.12f, 0.12f, 16);                       // shaft (starts inside the base)
		AddCylinderZ(m, 0.055f, 0.00f, 0.03f, 20);                        // collar against the base
		AddCylinderZ(m, 0.045f, 0.12f, 0.16f, 16);                        // hub
		AddBox(m, glm::vec3(0.0f, 0.0f, 0.145f), glm::vec3(0.19f, 0.028f, 0.015f));   // cross bar
		AddCylinderZ(m, 0.075f, 0.130f, 0.160f, 24, -0.19f, 0.0f);                    // left round paddle
		AddCylinderZ(m, 0.075f, 0.130f, 0.160f, 24, 0.19f, 0.0f);                     // right round paddle
	}
}

void WindingKey::Init()
{
	shader = new Shader("shaders/brass.vert", "shaders/brass.frag");   // procedural brass, no texture file

	Mesh mesh;
	BuildKey(mesh);
	indexCount = (GLsizei)mesh.indices.size();

	vao = new VAO();
	vao->Bind();
	VBO vbo(mesh.vertices.data(), (GLsizeiptr)(mesh.vertices.size() * sizeof(GLfloat)));
	EBO ebo(mesh.indices.data(), (GLsizeiptr)(mesh.indices.size() * sizeof(GLuint)));
	vao->LinkAttrib(vbo, 0, 3, GL_FLOAT, 6 * sizeof(GLfloat), (void*)0);
	vao->LinkAttrib(vbo, 1, 3, GL_FLOAT, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
}

void WindingKey::Draw(const FrameContext& frame, Camera& camera)
{
	// The harder the shake, the faster the key turns
	angle += TURN_SPEED * frame.shakeIntensity * frame.dt;
	if (angle > 6.2831853f) angle = std::fmod(angle, 6.2831853f);

	const float phi = glm::radians(MOUNT_ANGLE_DEG);
	glm::vec3 outwards(std::sin(phi), 0.0f, std::cos(phi));

	glm::mat4 model = frame.globeModel;
	model = glm::translate(model, outwards * (MOUNT_R * frame.globeRadius) + glm::vec3(0.0f, MOUNT_Y * frame.globeRadius, 0.0f));
	model = glm::rotate(model, phi, glm::vec3(0, 1, 0));        // local +Z -> outwards
	model = glm::rotate(model, angle, glm::vec3(0, 0, 1));      // turn around the shaft

	shader->Activate();
	camera.Matrix(45.0f, frame.globeRadius * 0.01f, frame.globeRadius * 50.0f, *shader, "camMatrix");
	glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform3f(glGetUniformLocation(shader->ID, "camPos"), frame.camPos.x, frame.camPos.y, frame.camPos.z);
	glUniform3f(glGetUniformLocation(shader->ID, "lightDir"), 0.4f, 1.0f, 0.6f);
	glUniform3f(glGetUniformLocation(shader->ID, "lightColor"), 0.75f, 0.70f, 0.62f);
	glUniform3f(glGetUniformLocation(shader->ID, "ambientColor"), 0.35f, 0.37f, 0.45f);

	vao->Bind();
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
}
