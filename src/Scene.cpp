#include "Scene.h"

#include <cmath>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "shaderClass.h"
#include "Texture.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "SceneConstants.h"
#include "WindingKey.h"
#include "Interior.h"

// ---------------------------------------------------------------
// Wooden base (Person 4)
// A solid of revolution built from a profile (radius, height) given in
// globe-radius units. Vertex = position(3) normal(3) uv(2).
// ---------------------------------------------------------------
namespace
{
	struct ProfilePoint { float r, y; };

	void BuildBase(float R, std::vector<GLfloat>& vertices, std::vector<GLuint>& indices)
	{
		const float PI = 3.14159265358979f;
		const int slices = 48;
		const float tile = 0.7f * R;                       // world size of one wood texture tile

		// Top of the base sits just below the interior ground so the two never z-fight.
		const float topY = (GROUND_Y_RATIO - 0.01f) * R;

		// Closed profile, bottom centre -> bottom edge -> plinth -> step -> tapered side -> top centre.
		const ProfilePoint profile[] = {
			{ 0.00f * R, -1.50f * R },
			{ 1.20f * R, -1.50f * R },
			{ 1.20f * R, -1.28f * R },
			{ 1.06f * R, -1.28f * R },
			{ 0.92f * R, topY },
			{ 0.00f * R, topY },
		};
		const int segments = (int)(sizeof(profile) / sizeof(profile[0])) - 1;

		float arc = 0.0f;                                  // length travelled along the profile (for v)

		for (int s = 0; s < segments; s++)
		{
			const ProfilePoint a = profile[s];
			const ProfilePoint b = profile[s + 1];
			const float dr = b.r - a.r;
			const float dy = b.y - a.y;
			const float len = std::sqrt(dr * dr + dy * dy);

			// Outward normal of the profile segment: (radial, up) = (dy, -dr)
			const float nr = dy / len;
			const float ny = -dr / len;

			const bool flat = std::fabs(ny) > 0.7f;        // top / bottom faces: planar UV
			const float rAvg = 0.5f * (a.r + b.r);
			const float repeats = std::fmax(1.0f, std::round(2.0f * PI * rAvg / tile));

			const GLuint first = (GLuint)(vertices.size() / 8);

			for (int j = 0; j <= slices; j++)
			{
				const float t = 2.0f * PI * j / slices;
				const float c = std::cos(t), sn = std::sin(t);

				const ProfilePoint p[2] = { a, b };
				const float segArc[2] = { arc, arc + len };

				for (int k = 0; k < 2; k++)
				{
					const float x = p[k].r * c, z = p[k].r * sn;
					float u, v;
					if (flat) { u = x / tile; v = z / tile; }
					else      { u = repeats * j / slices; v = segArc[k] / tile; }

					vertices.insert(vertices.end(), { x, p[k].y, z, nr * c, ny, nr * sn, u, v });
				}
			}

			// Per slice: a = start(j), b = end(j), c = start(j+1), d = end(j+1)
			for (int j = 0; j < slices; j++)
			{
				const GLuint a0 = first + 2 * j, b0 = a0 + 1, c0 = a0 + 2, d0 = a0 + 3;
				indices.insert(indices.end(), { a0, b0, d0, a0, d0, c0 });
			}
			arc += len;
		}
	}
}


void Scene::Init()
{
	shader = new Shader("shaders/default.vert", "shaders/default.frag");

	// Wooden base: textured shader + texture + geometry
	baseShader = new Shader("shaders/textured.vert", "shaders/textured.frag");
	woodTexture = new Texture();
	woodTexture->Load("textures/wood.png", GL_REPEAT, true);   // nearest = crisp pixel look
	baseShader->Activate();
	Texture::Uniform(*baseShader, "tex0", 0);

	{
		std::vector<GLfloat> vertices;
		std::vector<GLuint> indices;
		BuildBase(1.0f, vertices, indices);                    // globe radius is 1 (see main.cpp)
		baseIndexCount = (GLsizei)indices.size();

		baseVAO = new VAO();
		baseVAO->Bind();
		VBO vbo(vertices.data(), (GLsizeiptr)(vertices.size() * sizeof(GLfloat)));
		EBO ebo(indices.data(), (GLsizeiptr)(indices.size() * sizeof(GLuint)));
		baseVAO->LinkAttrib(vbo, 0, 3, GL_FLOAT, 8 * sizeof(GLfloat), (void*)0);
		baseVAO->LinkAttrib(vbo, 1, 3, GL_FLOAT, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
		baseVAO->LinkAttrib(vbo, 2, 2, GL_FLOAT, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
	}

	windingKey = new WindingKey();
	windingKey->Init();

	interior = new Interior();
	interior->Init();
}

void Scene::DrawOpaque(const FrameContext& frame, Camera& camera)
{
	glDepthMask(GL_TRUE);
	shader->Activate();
	camera.Matrix(45.0f, frame.globeRadius * 0.01f, frame.globeRadius * 50.0f, *shader, "camMatrix");

	// Wooden base. It shakes together with the globe (frame.globeModel).
	baseShader->Activate();
	camera.Matrix(45.0f, frame.globeRadius * 0.01f, frame.globeRadius * 50.0f, *baseShader, "camMatrix");
	glUniformMatrix4fv(glGetUniformLocation(baseShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(frame.globeModel));
	OutsideLightValues outside = OutsideLight(frame.daylight);     // same as before at night
	glUniform3fv(glGetUniformLocation(baseShader->ID, "lightDir"), 1, glm::value_ptr(outside.dir));
	glUniform3fv(glGetUniformLocation(baseShader->ID, "lightColor"), 1, glm::value_ptr(outside.color));
	glUniform3fv(glGetUniformLocation(baseShader->ID, "ambientColor"), 1, glm::value_ptr(outside.ambient));
	woodTexture->Bind(0);
	baseVAO->Bind();
	glDrawElements(GL_TRIANGLES, baseIndexCount, GL_UNSIGNED_INT, 0);

	// Winding key on the base: turns when the globe is shaken
	windingKey->Draw(frame, camera);

	// Interior: ground, trees, hut, campfire, character (Person 3)
	interior->Draw(frame, camera);
}

void Scene::Update(const FrameContext& frame)
{
	interior->Update(frame);
}

bool Scene::Inputs(unsigned char key)
{
	return interior->Inputs(key);
}

const SceneLights& Scene::Lights() const
{
	return interior->Lights();
}
