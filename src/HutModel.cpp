// Log cabin (Person 3). Built in the hut's own frame (X = width, Z = depth, door on +Z),
// then placed with one transform. Detailed on every side: windows on all four walls,
// door with a wreath and a lantern at the front, stone chimney on the right,
// woodpile and chopping stump at the back, snow and icicles on the roof.

#include "InteriorModels.h"

#include <cmath>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

#include "InteriorLayout.h"
#include "SceneConstants.h"

using namespace layout;

namespace
{
	float Hash(float n) { float s = std::sin(n * 78.233f) * 43758.5453f; return s - std::floor(s); }

	const glm::vec3 X(1, 0, 0), Y(0, 1, 0), Z(0, 0, 1);
	const glm::vec3 GREEN(0.21f, 0.42f, 0.28f);           // door and shutters
	const glm::vec3 CREAM(0.86f, 0.80f, 0.66f);           // window frames

	struct Opening { float u0, u1, y0, y1; };              // along the wall, heights

	Material WindowPane()
	{
		Material m = palette::Plain({ 0.09f, 0.11f, 0.15f }, 0.9f, 90.0f);
		m.windowEmit = 1.0f;
		return m;
	}

	Material Ice()
	{
		return palette::Plain({ 0.70f, 0.82f, 0.95f }, 0.9f, 80.0f);
	}

	// One wall of logs along the wall's own +X. Courses that cross an opening are split.
	void LogWall(MeshBuilder& mb, const glm::mat4& wall, float length, const std::vector<Opening>& holes, int seed)
	{
		for (int i = 0; i < LOG_COURSES; i++)
		{
			float y = LOG_R + i * LOG_STEP;
			std::vector<glm::vec2> parts = { { -0.5f * length, 0.5f * length } };
			for (const Opening& o : holes)
			{
				if (y + 0.6f * LOG_R < o.y0 || y - 0.6f * LOG_R > o.y1) continue;
				std::vector<glm::vec2> cut;
				for (glm::vec2 p : parts)
				{
					if (o.u0 > p.x) cut.push_back({ p.x, std::min(p.y, o.u0) });
					if (o.u1 < p.y) cut.push_back({ std::max(p.x, o.u1), p.y });
				}
				parts = cut;
			}
			for (glm::vec2 p : parts)
			{
				if (p.y - p.x < 0.005f) continue;
				Material bark = palette::Bark();
				bark.color *= 0.85f + 0.3f * Hash(seed * 31.0f + i * 7.0f + p.x * 13.0f);
				mb.Log(glm::translate(wall, glm::vec3(0.5f * (p.x + p.y), y, 0.0f)), LOG_R, p.y - p.x,
					bark, palette::LogEnds(), 12);
			}
		}
	}

	// Rectangle of frame boards around an opening. f: origin at the opening centre on the
	// wall line, +Z out of the wall.
	void Frame(MeshBuilder& mb, const glm::mat4& f, float w, float h, bool bottom, const Material& m)
	{
		const float t = 0.0085f, d = LOG_R * 1.15f;
		mb.Box(glm::translate(f, glm::vec3(0, 0.5f * h + 0.5f * t, 0)), glm::vec3(0.5f * w + t, 0.5f * t, d), m);
		mb.Box(glm::translate(f, glm::vec3(-0.5f * w - 0.5f * t, 0, 0)), glm::vec3(0.5f * t, 0.5f * h, d), m);
		mb.Box(glm::translate(f, glm::vec3(0.5f * w + 0.5f * t, 0, 0)), glm::vec3(0.5f * t, 0.5f * h, d), m);
		if (bottom)
			mb.Box(glm::translate(f, glm::vec3(0, -0.5f * h - 0.5f * t, 0)), glm::vec3(0.5f * w + t, 0.5f * t, d), m);
	}

	void Window(MeshBuilder& mb, const glm::mat4& f, float w, float h, bool shutters,
		InteriorAnchors& anchors, const glm::mat4& toGlobe)
	{
		Material cream = palette::Plain(CREAM, 0.15f, 20.0f);
		mb.Box(glm::translate(f, glm::vec3(0, 0, 0.25f * LOG_R)), glm::vec3(0.5f * w, 0.5f * h, 0.003f), WindowPane());
		Frame(mb, f, w, h, true, cream);

		// cross bars over the glass
		float zb = 0.25f * LOG_R + 0.004f;
		mb.Box(glm::translate(f, glm::vec3(0, 0, zb)), glm::vec3(0.0022f, 0.5f * h, 0.0025f), cream);
		mb.Box(glm::translate(f, glm::vec3(0, 0, zb)), glm::vec3(0.5f * w, 0.0022f, 0.0025f), cream);

		// sill with a strip of snow on it
		float sillY = -0.5f * h - 0.0135f;
		mb.Box(glm::translate(f, glm::vec3(0, sillY, 0.7f * LOG_R)), glm::vec3(0.5f * w + 0.014f, 0.0045f, 1.5f * LOG_R), cream);
		mb.Sphere(glm::scale(glm::translate(f, glm::vec3(0, sillY + 0.0045f, 0.9f * LOG_R)),
			glm::vec3(0.5f * w + 0.010f, 0.0075f, 1.25f * LOG_R)), 1.0f, 4, 14, palette::Snow(), 0.0f, 1.0f);

		if (shutters)
		{
			Material green = palette::Planks(GREEN);
			float sw = 0.5f * w + 0.004f;
			for (int side = -1; side <= 1; side += 2)
				mb.Box(glm::translate(f, glm::vec3(side * (0.5f * w + 0.0095f + 0.5f * sw), 0, LOG_R + 0.004f)),
					glm::vec3(0.5f * sw, 0.5f * h + 0.006f, 0.0032f), green);
		}

		// warm light just in front of the glass (shines on the snow below the window)
		anchors.windowLights.push_back(glm::vec3(toGlobe * f * glm::vec4(0, -0.01f, 0.07f, 1)));
	}

	void Door(MeshBuilder& mb, const glm::mat4& f)
	{
		const float w = DOOR_W, h = DOOR_H;
		Material green = palette::Planks(GREEN);
		Material batten = palette::Planks(GREEN * 0.75f);
		mb.Box(glm::translate(f, glm::vec3(0, 0, 0.2f * LOG_R)), glm::vec3(0.5f * w, 0.5f * h, 0.006f), green);
		// Z brace: two battens and a diagonal
		float zb = 0.2f * LOG_R + 0.008f;
		for (int k = -1; k <= 1; k += 2)
			mb.Box(glm::translate(f, glm::vec3(0, k * 0.34f * h, zb)), glm::vec3(0.5f * w - 0.004f, 0.006f, 0.0025f), batten);
		float diag = std::atan2(0.68f * h, w - 0.012f);
		mb.Box(glm::rotate(glm::translate(f, glm::vec3(0, 0, zb)), diag, Z),
			glm::vec3(0.5f * std::hypot(0.68f * h, w - 0.012f), 0.005f, 0.0022f), batten);
		// handle
		mb.Sphere(glm::translate(f, glm::vec3(0.5f * w - 0.011f, -0.01f, zb + 0.004f)), 0.0045f, 5, 8, palette::Metal());
		Frame(mb, f, w, h, false, palette::Planks({ 0.40f, 0.26f, 0.16f }));

		// wreath with berries and a red bow
		glm::mat4 wr = glm::rotate(glm::translate(f, glm::vec3(0, 0.22f * h, zb + 0.008f)), 0.5f * layout::PI, X);
		Material leaves = palette::Needles();
		leaves.color = { 0.10f, 0.30f, 0.15f };
		mb.Torus(wr, 0.016f, 0.0055f, 18, 8, leaves);
		Material berry = palette::Plain({ 0.75f, 0.06f, 0.06f }, 0.6f, 50.0f);
		for (int k = 0; k < 7; k++)
		{
			float a = 2.0f * layout::PI * k / 7 + 0.3f;
			mb.Sphere(glm::translate(wr, glm::vec3(0.016f * std::cos(a), 0.005f, 0.016f * std::sin(a))), 0.0028f, 4, 6, berry);
		}
		Material bow = palette::Plain({ 0.70f, 0.08f, 0.08f }, 0.3f, 30.0f);
		glm::mat4 b = glm::translate(f, glm::vec3(0, 0.22f * h - 0.017f, zb + 0.013f));
		for (int side = -1; side <= 1; side += 2)
			mb.Box(glm::rotate(glm::translate(b, glm::vec3(side * 0.006f, 0, 0)), side * 0.5f, Z), glm::vec3(0.006f, 0.0035f, 0.002f), bow);

		// log doorstep with snow brushed off the middle
		mb.Log(glm::translate(f, glm::vec3(0, -0.5f * h + 0.004f, LOG_R + 0.018f)), 0.011f, w + 0.03f,
			palette::Bark(), palette::LogEnds(), 10);
	}

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

	void Chimney(MeshBuilder& mb, const glm::mat4& hut, InteriorAnchors& anchors, const glm::mat4& toGlobe)
	{
		const float cx = 0.5f * HUT_W + LOG_R + 0.028f, cz = -0.07f;
		const float top = WALL_H + ROOF_RISE + 0.06f;
		const float course = 0.029f;
		const int n = (int)(top / course);
		Material mortar = palette::Stone({ 0.30f, 0.29f, 0.30f });
		mb.Box(glm::translate(hut, glm::vec3(cx, 0.5f * top, cz)), glm::vec3(0.026f, 0.5f * top, 0.034f), mortar);
		for (int i = 0; i < n; i++)
		{
			// two stones per course, joints alternate like brickwork
			float y = (i + 0.5f) * course;
			float split = (i % 2 ? 0.3f : -0.3f) * 0.034f;
			for (int k = 0; k < 2; k++)
			{
				float z0 = k ? split : -0.034f, z1 = k ? 0.034f : split;
				float g = 0.48f + 0.22f * Hash(i * 3.0f + k);
				glm::mat4 m = glm::translate(hut, glm::vec3(cx + 0.001f * Hash(i + k + 9.0f), y, cz + 0.5f * (z0 + z1)));
				m = glm::rotate(m, 0.06f * (Hash(i * 5.0f + k) - 0.5f), Y);
				mb.Box(m, glm::vec3(0.0285f, 0.5f * course - 0.0016f, 0.5f * (z1 - z0) - 0.0016f),
					palette::Stone(glm::vec3(g * 1.06f, g * 0.98f, g * 0.90f)));
			}
		}
		// cap stone and the dark flue
		mb.Box(glm::translate(hut, glm::vec3(cx, top + 0.005f, cz)), glm::vec3(0.034f, 0.006f, 0.042f), palette::Stone());
		mb.Box(glm::translate(hut, glm::vec3(cx, top + 0.0105f, cz)), glm::vec3(0.016f, 0.001f, 0.022f),
			palette::Plain({ 0.03f, 0.03f, 0.03f }));
		anchors.chimneyTop = glm::vec3(toGlobe * hut * glm::vec4(cx, top + 0.015f, cz, 1));
	}

	void Roof(MeshBuilder& mb, const glm::mat4& hut)
	{
		const float halfSpan = 0.5f * HUT_W + ROOF_EAVE;
		const float alpha = std::atan(ROOF_SLOPE);
		const float slant = halfSpan / std::cos(alpha);
		const float depth = 0.5f * HUT_D + ROOF_GABLE;
		Material boards = palette::Planks({ 0.36f, 0.23f, 0.15f });
		Material snow = palette::Snow();

		for (int side = -1; side <= 1; side += 2)
		{
			// panel frame: X down the slope, Y = roof normal, origin at the ridge on the roof line
			glm::mat4 p = glm::translate(hut, glm::vec3(0, WALL_H + ROOF_RISE, 0));
			p = glm::rotate(p, -side * alpha, Z);
			if (side < 0) p = glm::rotate(p, layout::PI, Y);
			mb.Box(glm::translate(p, glm::vec3(0.5f * slant, 0.5f * ROOF_T, 0)), glm::vec3(0.5f * slant, 0.5f * ROOF_T, depth), boards);
			mb.Box(glm::translate(p, glm::vec3(0.5f * slant, ROOF_T + 0.5f * ROOF_SNOW_T, 0)),
				glm::vec3(0.5f * slant, 0.5f * ROOF_SNOW_T, depth - 0.006f), snow);
			// rounded snow lip hanging over the eave
			glm::mat4 lip = glm::translate(p, glm::vec3(slant - 0.002f, ROOF_T + 0.4f * ROOF_SNOW_T, 0));
			mb.Cylinder(glm::translate(glm::rotate(lip, 0.5f * layout::PI, X), glm::vec3(0, -(depth - 0.006f), 0)),
				0.014f, 0.014f, 2.0f * (depth - 0.006f), 12, snow, &snow);

			// icicles under the eave
			for (int k = 0; k < 13; k++)
			{
				float z = -depth + 0.02f + (2.0f * depth - 0.04f) * k / 12.0f + 0.006f * (Hash(k + side * 17.0f) - 0.5f);
				float len = 0.012f + 0.03f * Hash(k * 3.0f + side);
				glm::vec3 at = glm::vec3(p * glm::vec4(slant - 0.006f, -0.001f, z, 1));
				glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.0f), at), glm::vec3(1, -1, 1));
				mb.Cylinder(m, 0.0042f, 0.0f, len, 6, Ice());
			}
		}
		// ridge log with a snow roll on top
		mb.Log(glm::rotate(glm::translate(hut, glm::vec3(0, WALL_H + ROOF_RISE + 0.008f, 0)), 0.5f * layout::PI, Y),
			0.014f, 2.0f * depth, palette::Bark(), palette::LogEnds(), 10);
		glm::mat4 roll = glm::translate(hut, glm::vec3(0, WALL_H + ROOF_RISE + ROOF_T + 0.013f, -(depth - 0.006f)));
		mb.Cylinder(glm::rotate(roll, 0.5f * layout::PI, X), 0.019f, 0.019f, 2.0f * (depth - 0.006f), 12, snow, &snow);
	}

	void Gable(MeshBuilder& mb, const glm::mat4& f, bool roundWindow)
	{
		std::vector<glm::vec2> tri = {
			{ -0.5f * HUT_W - LOG_R, WALL_H - 0.004f },
			{ 0.5f * HUT_W + LOG_R, WALL_H - 0.004f },
			{ 0.0f, WALL_H + ROOF_RISE + 0.004f } };
		mb.Prism(f, tri, 0.022f, palette::Planks({ 0.52f, 0.36f, 0.22f }));
		if (!roundWindow) return;
		glm::mat4 w = glm::rotate(glm::translate(f, glm::vec3(0, WALL_H + 0.058f, 0.011f)), 0.5f * layout::PI, X);
		const Material pane = WindowPane();
		mb.Cylinder(w, 0.021f, 0.021f, 0.002f, 20, pane, &pane);
		Material cream = palette::Plain(CREAM, 0.15f, 20.0f);
		mb.Torus(glm::translate(w, glm::vec3(0, 0.002f, 0)), 0.0225f, 0.0035f, 20, 6, cream);
		mb.Box(glm::translate(w, glm::vec3(0, 0.004f, 0)), glm::vec3(0.021f, 0.0015f, 0.0018f), cream);
		mb.Box(glm::translate(w, glm::vec3(0, 0.004f, 0)), glm::vec3(0.0018f, 0.0015f, 0.021f), cream);
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

void BuildHut(MeshBuilder& mb, InteriorAnchors& anchors)
{
	const glm::mat4 toGlobe(1.0f);
	const glm::mat4 hut = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(HUT_X, GROUND_Y_RATIO - HUT_SINK, HUT_Z)),
		HUT_YAW, Y);
	const float hw = 0.5f * HUT_W, hd = 0.5f * HUT_D, over = 0.03f;

	// Window sizes and openings (u = along the wall's own +X)
	const float wy0 = 0.072f, wy1 = 0.135f, wyc = 0.5f * (wy0 + wy1), wh = wy1 - wy0;
	const float frontWinU = 0.085f, frontWinW = 0.058f;
	const float backWinU = 0.07f, backWinW = 0.07f;          // back wall's +X = hut -X
	const float leftWinU = -0.04f, leftWinW = 0.08f;          // left wall's +X = hut +Z
	const float rightWinU = -0.10f, rightWinW = 0.06f;        // right wall's +X = hut -Z

	// Wall frames: origin on the wall line at ground, +X along the wall, +Z out of the hut
	const glm::mat4 front = glm::translate(hut, glm::vec3(0, 0, hd));
	const glm::mat4 back = glm::rotate(glm::translate(hut, glm::vec3(0, 0, -hd)), layout::PI, Y);
	const glm::mat4 left = glm::rotate(glm::translate(hut, glm::vec3(-hw, 0, 0)), -0.5f * layout::PI, Y);
	const glm::mat4 right = glm::rotate(glm::translate(hut, glm::vec3(hw, 0, 0)), 0.5f * layout::PI, Y);

	const float pad = 0.0085f;   // frame board width
	LogWall(mb, front, HUT_W + 2 * over, {
		{ DOOR_LX - 0.5f * DOOR_W - pad, DOOR_LX + 0.5f * DOOR_W + pad, -1.0f, DOOR_H },
		{ frontWinU - 0.5f * frontWinW - pad, frontWinU + 0.5f * frontWinW + pad, wy0, wy1 } }, 1);
	LogWall(mb, back, HUT_W + 2 * over, { { backWinU - 0.5f * backWinW - pad, backWinU + 0.5f * backWinW + pad, wy0, wy1 } }, 2);
	LogWall(mb, left, HUT_D + 2 * over, { { leftWinU - 0.5f * leftWinW - pad, leftWinU + 0.5f * leftWinW + pad, wy0, wy1 } }, 3);
	LogWall(mb, right, HUT_D + 2 * over, { { rightWinU - 0.5f * rightWinW - pad, rightWinU + 0.5f * rightWinW + pad, wy0, wy1 } }, 4);

	// dark core: nothing can be seen through gaps between logs and frames
	mb.Box(glm::translate(hut, glm::vec3(0, 0.5f * WALL_H, 0)), glm::vec3(hw - 0.5f * LOG_R, 0.5f * WALL_H, hd - 0.5f * LOG_R),
		palette::Plain({ 0.10f, 0.07f, 0.05f }));

	Window(mb, glm::translate(front, glm::vec3(frontWinU, wyc, 0)), frontWinW, wh, false, anchors, toGlobe);
	Window(mb, glm::translate(back, glm::vec3(backWinU, wyc, 0)), backWinW, wh, true, anchors, toGlobe);
	Window(mb, glm::translate(left, glm::vec3(leftWinU, wyc, 0)), leftWinW, wh, true, anchors, toGlobe);
	Window(mb, glm::translate(right, glm::vec3(rightWinU, wyc, 0)), rightWinW, wh, true, anchors, toGlobe);
	Door(mb, glm::translate(front, glm::vec3(DOOR_LX, 0.5f * DOOR_H, 0)));
	Lantern(mb, glm::translate(front, glm::vec3(DOOR_LX - 0.5f * DOOR_W - 0.03f, 0.125f, LOG_R)), anchors, toGlobe);

	Gable(mb, front, true);
	Gable(mb, back, false);
	Roof(mb, hut);
	Chimney(mb, hut, anchors, toGlobe);
	Woodpile(mb, glm::translate(back, glm::vec3(-0.065f, 0.0f, LOG_R + 0.055f)));
	ChoppingStump(mb, glm::rotate(glm::translate(back, glm::vec3(-0.21f, -0.005f, 0.11f)), 0.8f, Y));

	// snow banked against the walls
	Material snow = palette::Snow();
	struct Bank { const glm::mat4* wall; float u, w; };
	const Bank banks[] = {
		{ &front, 0.11f, 0.07f }, { &back, -0.12f, 0.05f }, { &back, 0.09f, 0.06f },
		{ &left, 0.10f, 0.08f }, { &left, -0.13f, 0.05f }, { &right, 0.05f, 0.06f }, { &right, -0.16f, 0.04f } };
	for (const Bank& b : banks)
		mb.Sphere(glm::scale(glm::translate(*b.wall, glm::vec3(b.u, HUT_SINK - 0.002f, LOG_R + 0.004f)),
			glm::vec3(b.w, 0.028f, 0.026f)), 1.0f, 5, 14, snow, 0.0f, 1.0f);
}
