#include "Globe.h"

#include <vector>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Cubemap.h"

const float GLASS_ALPHA = 0.2f;
const float CUBEMAP_STRENGTH = 0.6f;
const float CUBEMAP_STRENGTH_INSIDE = 0.25f;
const glm::vec3 GLASS_TINT(0.75f, 0.88f, 1.0f);
// Temporary light direction; Person 4/3 will later feed real fire/window lights.
const glm::vec3 GLASS_LIGHT_DIR = glm::normalize(glm::vec3(-0.4f, 0.8f, 0.5f));

// ---------------------------------------------------------------
// Simple UV sphere (position + normal, interleaved). Used as the globe for now.
// ---------------------------------------------------------------
static void buildSphere(float radius, int stacks, int slices,
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
			// Counter-clockwise when viewed from outside the sphere.
			indices.insert(indices.end(), { a, a + 1, b, a + 1, b + 1, b });
		}
}

void Globe::Init(float radius)
{
	shader = new Shader("shaders/glass.vert", "shaders/glass.frag");
	skyCubemap = CreateNightSkyCubemap();
	skyBoxLocation = glGetUniformLocation(shader->ID, "skyBox");
	reflectStrengthLocation = glGetUniformLocation(shader->ID, "reflectStrength");
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

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
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(pass == GlassPass::Back ? GL_FRONT : GL_BACK);

	shader->Activate();
	camera.Matrix(45.0f, frame.globeRadius * 0.01f, frame.globeRadius * 50.0f, *shader, "camMatrix");

	glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(frame.globeModel));
	glUniform3fv(glGetUniformLocation(shader->ID, "camPos"), 1, glm::value_ptr(frame.camPos));
	glUniform3fv(glGetUniformLocation(shader->ID, "lightDir"), 1, glm::value_ptr(GLASS_LIGHT_DIR));
	glUniform3fv(glGetUniformLocation(shader->ID, "glassTint"), 1, glm::value_ptr(GLASS_TINT));
	glUniform1f(glGetUniformLocation(shader->ID, "glassAlpha"), GLASS_ALPHA);
	glUniform1f(reflectStrengthLocation,
		frame.cameraInside ? CUBEMAP_STRENGTH_INSIDE : CUBEMAP_STRENGTH);

	GLint previousUnit, previousCubemap;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &previousUnit);
	glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &previousCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyCubemap);
	glUniform1i(skyBoxLocation, 0);

	sphereVAO->Bind();
	glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, previousCubemap);
	glActiveTexture(previousUnit);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
}
