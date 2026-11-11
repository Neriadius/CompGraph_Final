#define DEBUG_TEST_OBJECT 1   // remove when the real interior is added

#include "Scene.h"

#include "Camera.h"
#include "shaderClass.h"

#if DEBUG_TEST_OBJECT
#include <cmath>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VAO.h"
#include "VBO.h"
#include "EBO.h"

static VAO* testVAO = nullptr;
static GLsizei testIndexCount = 0;
#endif

void Scene::Init()
{
	shader = new Shader("shaders/default.vert", "shaders/default.frag");

#if DEBUG_TEST_OBJECT
	const float PI = 3.14159265358979f;
	const int stacks = 16, slices = 24;
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	for (int i = 0; i <= stacks; i++)
	{
		float phi = PI * i / stacks;
		for (int j = 0; j <= slices; j++)
		{
			float theta = 2.0f * PI * j / slices;
			// Constant normal gives a flat grey colour with the unchanged default shader.
			vertices.insert(vertices.end(), {
				0.3f * std::sin(phi) * std::cos(theta),
				0.3f * std::cos(phi),
				0.3f * std::sin(phi) * std::sin(theta), 0.0f, 1.0f, 0.0f });
		}
	}
	for (int i = 0; i < stacks; i++)
		for (int j = 0; j < slices; j++)
		{
			GLuint a = i * (slices + 1) + j;
			GLuint b = a + slices + 1;
			indices.insert(indices.end(), { a, a + 1, b, a + 1, b + 1, b });
		}
	testIndexCount = (GLsizei)indices.size();
	testVAO = new VAO();
	testVAO->Bind();
	VBO vbo(vertices.data(), (GLsizeiptr)(vertices.size() * sizeof(GLfloat)));
	EBO ebo(indices.data(), (GLsizeiptr)(indices.size() * sizeof(GLuint)));
	testVAO->LinkAttrib(vbo, 0, 3, GL_FLOAT, 6 * sizeof(GLfloat), (void*)0);
	testVAO->LinkAttrib(vbo, 1, 3, GL_FLOAT, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
#endif
}

void Scene::DrawOpaque(const FrameContext& frame, Camera& camera)
{
	glDepthMask(GL_TRUE);
	shader->Activate();
	camera.Matrix(45.0f, frame.globeRadius * 0.01f, frame.globeRadius * 50.0f, *shader, "camMatrix");

	// Opaque interior and base will be drawn here.
#if DEBUG_TEST_OBJECT
	glm::mat4 model = glm::translate(frame.globeModel, glm::vec3(0.3f, -0.4f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
	testVAO->Bind();
	glDrawElements(GL_TRIANGLES, testIndexCount, GL_UNSIGNED_INT, 0);
#endif
}
