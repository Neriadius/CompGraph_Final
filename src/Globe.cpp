#include "Globe.h"

#include <vector>
#include <cmath>

#include "Camera.h"
#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"

// ---------------------------------------------------------------
// Simple UV sphere (position + normal, interleaved). Used as the globe for now.
// ---------------------------------------------------------------
void buildSphere(float radius, int stacks, int slices,
	std::vector<GLfloat>& vertices, std::vector<GLuint>& indices)
{
	const float PI = 3.14159265358979f;

	for (int i = 0; i <= stacks; i++)
	{
		float phi = PI * i / stacks;                 // 0 (top) .. PI (bottom)
		for (int j = 0; j <= slices; j++)
		{
			float theta = 2.0f * PI * j / slices;
			float nx = std::sin(phi) * std::cos(theta);
			float ny = std::cos(phi);
			float nz = std::sin(phi) * std::sin(theta);

			vertices.insert(vertices.end(), { nx * radius, ny * radius, nz * radius, nx, ny, nz });
		}
	}

	for (int i = 0; i < stacks; i++)
		for (int j = 0; j < slices; j++)
		{
			GLuint a = i * (slices + 1) + j;
			GLuint b = a + slices + 1;
			indices.insert(indices.end(), { a, b, a + 1, a + 1, b, b + 1 });
		}
}

void Globe::Init(float radius)
{
	shader = new Shader("shaders/default.vert", "shaders/default.frag");

	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	buildSphere(radius, 32, 48, vertices, indices);
	sphereIndexCount = (GLsizei)indices.size();

	sphereVAO = new VAO();
	sphereVAO->Bind();
	sphereVBO = new VBO(vertices.data(), (GLsizeiptr)(vertices.size() * sizeof(GLfloat)));
	sphereEBO = new EBO(indices.data(), (GLsizeiptr)(indices.size() * sizeof(GLuint)));
	sphereVAO->LinkAttrib(*sphereVBO, 0, 3, GL_FLOAT, 6 * sizeof(GLfloat), (void*)0);
	sphereVAO->LinkAttrib(*sphereVBO, 1, 3, GL_FLOAT, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
}

void Globe::DrawGlass(GlassPass pass, const FrameContext& frame, Camera& camera)
{
	if (pass == GlassPass::Back) return;

	shader->Activate();
	camera.Matrix(45.0f, frame.globeRadius * 0.01f, frame.globeRadius * 50.0f, *shader, "camMatrix");

	glm::vec3 jitter(
		std::sin(frame.time * 47.0f),
		std::sin(frame.time * 61.0f),
		std::sin(frame.time * 53.0f));
	glm::mat4 model = glm::translate(glm::mat4(1.0f), jitter * (0.03f * frame.shakeIntensity));
	glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

	sphereVAO->Bind();
	glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
}
