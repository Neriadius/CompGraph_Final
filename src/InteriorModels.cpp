#include "InteriorModels.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "InteriorLayout.h"
#include "SceneConstants.h"
#include "SceneLights.h"

using namespace layout;


// Palette
namespace palette
{
	Material Snow()
	{
		Material m; m.color = { 0.90f, 0.93f, 1.00f }; m.spec = 0.18f; m.shininess = 30.0f;
		m.pattern = PAT_SNOW; m.patternAmount = 1.0f; return m;
	}
	Material PackedSnow()
	{
		Material m = Snow(); m.color = { 0.70f, 0.73f, 0.84f }; return m;
	}
	Material Bark()
	{
		Material m; m.color = { 0.46f, 0.29f, 0.17f }; m.spec = 0.05f; m.shininess = 10.0f;
		m.pattern = PAT_BARK; m.patternAmount = 1.0f; return m;
	}
	Material LogEnds()
	{
		Material m; m.color = { 0.82f, 0.63f, 0.40f }; m.spec = 0.04f; m.shininess = 8.0f;
		m.pattern = PAT_RINGS; m.patternAmount = 1.0f; return m;
	}
	Material Stone(const glm::vec3& color)
	{
		Material m; m.color = color; m.spec = 0.12f; m.shininess = 18.0f;
		m.pattern = PAT_STONE; m.patternAmount = 1.0f; return m;
	}
	Material Metal()
	{
		Material m; m.color = { 0.20f, 0.21f, 0.23f }; m.spec = 0.7f; m.shininess = 60.0f; return m;
	}
	Material Plain(const glm::vec3& color, float spec, float shininess)
	{
		Material m; m.color = color; m.spec = spec; m.shininess = shininess; return m;
	}
}

namespace
{
	float Hash(float n) { float s = std::sin(n * 12.9898f) * 43758.5453f; return s - std::floor(s); }

	glm::mat4 At(float x, float y, float z, float yaw = 0.0f)
	{
		return glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)), yaw, glm::vec3(0, 1, 0));
	}
}

float GroundY(float x, float z)
{
	return GROUND_Y_RATIO + TerrainOffset(x, z) - REST_LIFT;
}

// How much of a light reaches point p (1 = lit, 0 = behind a mountain)
static float TerrainVisibility(const glm::vec3& p, const glm::vec3& L)
{
	float vis = 1.0f;
	for (float t = 0.012f; t < 1.2f; t += 0.006f + 0.03f * t)
	{
		glm::vec3 q = p + L * t;
		if (q.x * q.x + q.z * q.z > GROUND_DISC_R * GROUND_DISC_R) break;
		float h = GROUND_Y_RATIO + TerrainOffset(q.x, q.z);
		vis = std::min(vis, (q.y - h) / (0.05f * t + 0.004f));
		if (vis <= 0.0f) return 0.0f;
	}
	return vis;
}

// Ground: polar grid over the terrain height map (mountains, drifts), with a packed path,
// a scorched ring, and mountain shadows for the moon and the sun baked into uv
void BuildGround(MeshBuilder& mb)
{
	const int rings = 110, slices = 240;
	GLuint first = mb.VertexCount();

	auto pos = [&](int i, int j) {
		float r = GROUND_DISC_R * std::pow((float)i / (rings - 1), 0.85f);   // denser near the centre
		float a = 2.0f * PI * j / slices;
		float x = r * std::cos(a), z = r * std::sin(a);
		return glm::vec3(x, GroundY(x, z), z);
	};
	auto uv = [&](int i, int j) {
		glm::vec3 p = pos(i, j);
		p.y += REST_LIFT + 0.004f;                         // start on the surface, avoid self-shadowing
		return glm::vec2(TerrainVisibility(p, sky::MOON_DIR), TerrainVisibility(p, sky::SUN_DIR));
	};
	Material ground = palette::Snow();
	ground.pattern = PAT_TERRAIN;
	mb.Grid(rings, slices, true, pos, uv, ground);

	// Path from the hut door to the fire, packed snow around the fire, scorched ring
	float doorX, doorZ;
	FromHut(DOOR_LX, HUT_FRONT + 0.04f, doorX, doorZ);
	const glm::vec2 a(doorX, doorZ), b(FIRE_X, FIRE_Z), seat(CHAR_X, CHAR_Z), stump(STUMP_X, STUMP_Z);
	const glm::vec3 packed = palette::PackedSnow().color, scorch(0.30f, 0.27f, 0.27f);

	auto segDist = [](glm::vec2 p, glm::vec2 s0, glm::vec2 s1) {
		glm::vec2 d = s1 - s0;
		float t = glm::clamp(glm::dot(p - s0, d) / glm::dot(d, d), 0.0f, 1.0f);
		return glm::length(p - (s0 + d * t));
	};

	for (GLuint v = first; v < mb.VertexCount(); v++)
	{
		GLfloat* f = &mb.vertices[(size_t)v * MeshBuilder::FLOATS_PER_VERTEX];
		glm::vec2 p(f[0], f[2]);
		float wobble = 0.008f * std::sin(p.x * 70.0f) * std::cos(p.y * 55.0f);
		float path = std::min({ segDist(p, a, b), segDist(p, b, seat), segDist(p, b, stump) }) + wobble;
		float k = 1.0f - Smooth(0.022f, 0.040f, path);
		float fireD = glm::length(p - b);
		k = std::max(k, 1.0f - Smooth(0.10f, 0.15f, fireD));
		glm::vec3 c = glm::mix(palette::Snow().color, packed, k);
		c = glm::mix(c, scorch, (1.0f - Smooth(0.055f, 0.085f, fireD)) * 0.8f);
		f[6] = c.r; f[7] = c.g; f[8] = c.b;
	}
}

// Fire pit: ash, stone ring, crossed + leaning logs, glowing coals
void BuildFirePit(MeshBuilder& mb, InteriorAnchors& anchors)
{
	const float gy = GroundY(FIRE_X, FIRE_Z);
	const glm::mat4 root = At(FIRE_X, gy, FIRE_Z);
	anchors.fire = glm::vec3(FIRE_X, gy, FIRE_Z);

	Material ash = palette::Stone({ 0.20f, 0.19f, 0.19f });
	ash.patternAmount = 0.6f;
	mb.Cylinder(glm::translate(root, glm::vec3(0, -0.004f, 0)), 0.060f, 0.056f, 0.010f, 24, ash, &ash);

	// stone ring: low-poly squashed spheres
	for (int k = 0; k < 10; k++)
	{
		float a = 2.0f * PI * k / 10 + 0.2f * Hash(k + 1.0f);
		float r = 0.068f + 0.004f * Hash(k + 2.0f);
		glm::vec3 grey(0.42f + 0.18f * Hash(k + 3.0f));
		glm::mat4 m = glm::translate(root, glm::vec3(r * std::cos(a), 0.004f, r * std::sin(a)));
		m = glm::rotate(m, Hash(k + 4.0f) * PI, glm::vec3(0, 1, 0));
		m = glm::scale(m, glm::vec3(1.25f, 0.75f, 0.95f) * (0.9f + 0.25f * Hash(k + 5.0f)));
		size_t firstIndex = mb.indices.size();
		mb.Sphere(m, 0.016f, 4, 7, palette::Stone(grey * glm::vec3(1.0f, 0.98f, 1.03f)));
		mb.Facet(firstIndex);
	}

	Material charred = palette::Bark();
	charred.color = { 0.16f, 0.11f, 0.08f };
	Material hotEnds = palette::LogEnds();
	hotEnds.color = { 0.25f, 0.12f, 0.06f };
	hotEnds.fireEmit = 0.7f;

	// two logs lying crossed under the fire
	for (int k = 0; k < 2; k++)
	{
		glm::mat4 m = glm::rotate(glm::translate(root, glm::vec3(0, 0.010f, 0)), (0.35f + k * 1.75f), glm::vec3(0, 1, 0));
		mb.Log(m, 0.0105f, 0.095f, charred, hotEnds, 10);
	}

	// leaning logs
	for (int k = 0; k < 4; k++)
	{
		float a = 2.0f * PI * k / 4 + 0.6f;
		glm::vec3 base(0.048f * std::cos(a), 0.004f, 0.048f * std::sin(a));
		glm::vec3 top(0.010f * std::cos(a + 0.4f), 0.062f, 0.010f * std::sin(a + 0.4f));
		float len;
		glm::mat4 m = root * AlignY(base, top, &len);
		Material ends = (k % 2) ? hotEnds : palette::LogEnds();
		mb.Cylinder(m, 0.0085f, 0.0075f, len, 9, charred, &ends);
	}

	// glowing coals
	Material coal = palette::Plain({ 0.14f, 0.07f, 0.05f });
	coal.fireEmit = 1.0f;
	for (int k = 0; k < 9; k++)
	{
		float a = 2.0f * PI * Hash(k + 11.0f), r = 0.034f * std::sqrt(Hash(k + 12.0f));
		glm::mat4 m = glm::translate(root, glm::vec3(r * std::cos(a), 0.006f, r * std::sin(a)));
		m = glm::rotate(m, Hash(k + 13.0f) * PI, glm::vec3(0.3f, 1, 0.2f));
		size_t firstIndex = mb.indices.size();
		mb.Sphere(glm::scale(m, glm::vec3(1.2f, 0.7f, 1.0f)), 0.009f, 3, 6, coal);
		mb.Facet(firstIndex);
	}
}

// Seats: the log the character sits on, and a stump with a tin mug
void BuildSeats(MeshBuilder& mb)
{
	Material bark = palette::Bark(), ends = palette::LogEnds();

	glm::mat4 seat = At(CHAR_X, GroundY(CHAR_X, CHAR_Z), CHAR_Z, CHAR_YAW);
	mb.Log(glm::translate(seat, glm::vec3(0, 0.022f, -0.004f)), 0.026f, 0.15f, bark, ends, 14);
	// snow left on both ends of the seat
	Material snow = palette::Snow();
	for (int side = -1; side <= 1; side += 2)
		mb.Sphere(glm::scale(glm::translate(seat, glm::vec3(side * 0.058f, 0.047f, -0.004f)),
			glm::vec3(1.6f, 0.45f, 1.0f)), 0.016f, 5, 10, snow, 0.0f, 1.0f);

	glm::mat4 stump = At(STUMP_X, GroundY(STUMP_X, STUMP_Z) - 0.01f, STUMP_Z, 0.7f);
	mb.Cylinder(stump, 0.038f, 0.034f, 0.064f, 16, bark, &ends);
	// mug: body, rim, handle
	Material mug = palette::Plain({ 0.62f, 0.18f, 0.14f }, 0.45f, 40.0f);
	Material inside = palette::Plain({ 0.25f, 0.13f, 0.07f }, 0.5f, 40.0f);    // cocoa
	glm::mat4 cup = glm::translate(stump, glm::vec3(0.008f, 0.064f, -0.006f));
	mb.Cylinder(cup, 0.0095f, 0.0105f, 0.020f, 14, mug, &inside);
	glm::mat4 handle = glm::rotate(glm::translate(cup, glm::vec3(0.0115f, 0.010f, 0)), 0.5f * PI, glm::vec3(1, 0, 0));
	mb.Torus(handle, 0.0055f, 0.0018f, 12, 6, mug);
}
