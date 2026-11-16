// Props around the hut model (Person 3): lantern by the door, woodpile and chopping stump
// at the back, snow banked against the walls. The hut itself is models/hut/hut.obj
// (drawn by Interior.cpp with the same placement matrix, HutMatrix()).

#include "InteriorModels.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "InteriorLayout.h"
#include "SceneConstants.h"

using namespace layout;

namespace
{
	float Hash(float n) { float s = std::sin(n * 78.233f) * 43758.5453f; return s - std::floor(s); }

	const glm::vec3 Y(0, 1, 0), Z(0, 0, 1);

	void Lantern(MeshBuilder& mb, const glm::mat4& f, InteriorAnchors& anchors, const glm::mat4& toGlobe)
	{
		Material metal = palette::Metal();
		mb.Box(glm::translate(f, glm::vec3(0, 0.030f, 0.5f * 0.034f)), glm::vec3(0.0022f, 0.0022f, 0.017f), metal);   // arm
		mb.Box(glm::translate(f, glm::vec3(0, 0.022f, 0.032f)), glm::vec3(0.0012f, 0.008f, 0.0012f), metal);          // hook
		glm::mat4 body = glm::translate(f, glm::vec3(0, 0.0f, 0.032f));
		Material glass = palette::Plain({ 0.35f, 0.30f, 0.20f }, 0.8f, 80.0f);
		glass.windowEmit = 1.0f;
		mb.Box(body, glm::vec3(0.0085f, 0.012f, 0.0085f), glass);
		for (int k = 0; k < 4; k++)
		{
			float sx = (k & 1) ? 1.0f : -1.0f, sz = (k & 2) ? 1.0f : -1.0f;
			mb.Box(glm::translate(body, glm::vec3(sx * 0.0088f, 0, sz * 0.0088f)), glm::vec3(0.0016f, 0.0125f, 0.0016f), metal);
		}
		mb.Box(glm::translate(body, glm::vec3(0, -0.0135f, 0)), glm::vec3(0.0105f, 0.0018f, 0.0105f), metal);
		mb.Cylinder(glm::rotate(glm::translate(body, glm::vec3(0, 0.0118f, 0)), 0.25f * layout::PI, Y),
			0.0145f, 0.0f, 0.012f, 4, metal);
		anchors.lantern = glm::vec3(toGlobe * body * glm::vec4(0, 0, 0.004f, 1));
	}

	void Woodpile(MeshBuilder& mb, const glm::mat4& f)
	{
		// logs point away from the back wall so their rings face the viewer
		const float r = 0.0155f, len = 0.10f;
		const int rows[] = { 5, 4, 3, 2 };
		int n = 0;
		for (int row = 0; row < 4; row++)
			for (int k = 0; k < rows[row]; k++, n++)
			{
				float x = (k - 0.5f * (rows[row] - 1)) * 2.0f * r;
				float y = r + row * r * 1.732f;
				Material bark = palette::Bark();
				bark.color *= 0.8f + 0.35f * Hash(n + 50.0f);
				Material ends = palette::LogEnds();
				ends.color *= 0.9f + 0.15f * Hash(n + 70.0f);
				mb.Log(glm::rotate(glm::translate(f, glm::vec3(x, y, 0.004f * Hash(n + 3.0f))), 0.5f * layout::PI, Y),
					r, len, bark, ends, 10);
			}
		mb.Sphere(glm::scale(glm::translate(f, glm::vec3(0, r + 3 * r * 1.732f + 0.7f * r, 0)), glm::vec3(0.030f, 0.009f, 0.050f)),
			1.0f, 5, 12, palette::Snow(), 0.0f, 1.0f);
		for (int side = -1; side <= 1; side += 2)
			mb.Box(glm::translate(f, glm::vec3(side * 5.6f * r, 0.045f, 0)), glm::vec3(0.003f, 0.05f, 0.003f), palette::Bark());
	}

	void ChoppingStump(MeshBuilder& mb, const glm::mat4& f)
	{
		const Material ends = palette::LogEnds();
		mb.Cylinder(f, 0.03f, 0.027f, 0.045f, 14, palette::Bark(), &ends);
		// axe stuck in the top, handle leaning out
		glm::mat4 axe = glm::rotate(glm::translate(f, glm::vec3(0.006f, 0.045f, 0)), -0.5f, Z);
		mb.Box(glm::translate(axe, glm::vec3(0, 0.002f, 0)), glm::vec3(0.012f, 0.009f, 0.002f), palette::Metal());
		mb.Cylinder(glm::translate(axe, glm::vec3(0.006f, 0.004f, 0)) * glm::rotate(glm::mat4(1.0f), -1.2f, Z),
			0.0028f, 0.0025f, 0.075f, 6, palette::Plain({ 0.55f, 0.38f, 0.22f }));
	}
}

glm::mat4 HutMatrix()
{
	return glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(HUT_X, GROUND_Y_RATIO - HUT_SINK, HUT_Z)), HUT_YAW, Y);
}

void BuildHutProps(MeshBuilder& mb, InteriorAnchors& anchors)
{
	const glm::mat4 toGlobe(1.0f);
	const glm::mat4 hut = HutMatrix();

	// Wall frames: origin on the outer face of a wall at ground, +X along it, +Z out of the hut
	const glm::mat4 front = glm::translate(hut, glm::vec3(0, 0, HUT_FRONT));
	const glm::mat4 back = glm::rotate(glm::translate(hut, glm::vec3(0, 0, HUT_BACK)), PI, Y);
	const glm::mat4 left = glm::rotate(glm::translate(hut, glm::vec3(HUT_LEFT, 0, 0)), -0.5f * PI, Y);
	const glm::mat4 right = glm::rotate(glm::translate(hut, glm::vec3(HUT_RIGHT, 0, 0)), 0.5f * PI, Y);

	Lantern(mb, glm::translate(front, glm::vec3(DOOR_LX - 0.075f, 0.118f, 0.0f)), anchors, toGlobe);
	Woodpile(mb, glm::translate(back, glm::vec3(-0.08f, HUT_SINK - 0.002f, 0.058f)));
	ChoppingStump(mb, glm::rotate(glm::translate(back, glm::vec3(-0.235f, HUT_SINK - 0.006f, 0.10f)), 0.8f, Y));

	// lights from the model: outside the window, top of the chimney
	anchors.windowLights.push_back(glm::vec3(hut * glm::vec4(WINDOW_LIGHT[0], WINDOW_LIGHT[1], WINDOW_LIGHT[2], 1.0f)));
	anchors.chimneyTop = glm::vec3(hut * glm::vec4(CHIMNEY_TOP[0], CHIMNEY_TOP[1] + 0.012f, CHIMNEY_TOP[2], 1.0f));

	// snow banked against the walls (not in front of the door or under the window)
	Material snow = palette::Snow();
	struct Bank { const glm::mat4* wall; float u, w; };
	const Bank banks[] = {
		{ &front, 0.11f, 0.06f }, { &front, -0.14f, 0.04f }, { &back, 0.10f, 0.06f },
		{ &left, 0.10f, 0.08f }, { &left, -0.12f, 0.06f }, { &right, 0.12f, 0.05f }, { &right, -0.13f, 0.05f } };
	for (const Bank& b : banks)
		mb.Sphere(glm::scale(glm::translate(*b.wall, glm::vec3(b.u, HUT_SINK - 0.004f, 0.006f)),
			glm::vec3(b.w, 0.026f, 0.024f)), 1.0f, 5, 14, snow, 0.0f, 1.0f);
}
