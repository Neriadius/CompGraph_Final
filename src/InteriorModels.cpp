#include "InteriorModels.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "InteriorLayout.h"
#include "SceneConstants.h"

using namespace layout;

// ---------------------------------------------------------------
// Palette
// ---------------------------------------------------------------
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
	Material Planks(const glm::vec3& color)
	{
		Material m; m.color = color; m.spec = 0.05f; m.shininess = 10.0f;
		m.pattern = PAT_PLANKS; m.patternAmount = 1.0f; return m;
	}
	Material Stone(const glm::vec3& color)
	{
		Material m; m.color = color; m.spec = 0.12f; m.shininess = 18.0f;
		m.pattern = PAT_STONE; m.patternAmount = 1.0f; return m;
	}
	Material Needles()
	{
		Material m; m.color = { 0.12f, 0.33f, 0.20f }; m.spec = 0.06f; m.shininess = 12.0f;
		m.pattern = PAT_NEEDLES; m.patternAmount = 1.0f; return m;
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

// ---------------------------------------------------------------
// Snow ground: polar grid that follows the drifts, with a packed path and a scorched ring
// ---------------------------------------------------------------
void BuildGround(MeshBuilder& mb)
{
	const int rings = 56, slices = 144;
	GLuint first = mb.VertexCount();

	auto pos = [&](int i, int j) {
		float r = GROUND_DISC_R * std::pow((float)i / (rings - 1), 0.85f);   // denser near the centre
		float a = 2.0f * PI * j / slices;
		float x = r * std::cos(a), z = r * std::sin(a);
		return glm::vec3(x, GroundY(x, z), z);
	};
	auto uv = [&](int i, int j) { glm::vec3 p = pos(i, j); return glm::vec2(p.x, p.z); };
	mb.Grid(rings, slices, true, pos, uv, palette::Snow());

	// Path from the hut door to the fire, packed snow around the fire, scorched ring
	float doorX, doorZ;
	FromHut(DOOR_LX, 0.5f * HUT_D + 0.06f, doorX, doorZ);
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

// ---------------------------------------------------------------
// Pine trees: trunk + three tiers with wavy hems, each tier wearing a snow cap
// ---------------------------------------------------------------
namespace
{
	// Point on a tier's cone. theta = tree-local angle, u = 0 at the tip .. 1 at the hem,
	// out = push along the surface normal (for the snow cap).
	glm::vec3 TierPoint(const Tier& tier, float s, float seed, float theta, float u, float out)
	{
		float wave = TierWave(theta, seed);
		float R = tier.radius * s * wave;
		float H = tier.height * s;
		float top = (tier.bottom + tier.height) * s;
		float droop = 0.6f * H * std::pow(u, 4.0f) * std::max(0.0f, wave - 1.0f);   // tips bend down
		glm::vec3 p(R * u * std::cos(theta), top - H * u - droop, R * u * std::sin(theta));
		glm::vec3 n = glm::normalize(glm::vec3(H * std::cos(theta), R, H * std::sin(theta)));
		return p + n * out;
	}

	void BuildTree(MeshBuilder& mb, const TreeSpot& t, int treeIndex)
	{
		const float s = t.scale;
		// +yaw: the same rotation TreeTop() undoes when it looks up the wavy hem
		glm::mat4 root = At(t.x, GROUND_Y_RATIO + TerrainOffset(t.x, t.z), t.z, t.yawDeg * DEG);
		Material ends = palette::LogEnds();
		Material trunk = palette::Bark();
		trunk.color = { 0.32f, 0.21f, 0.14f };
		mb.Cylinder(glm::translate(root, glm::vec3(0, -0.03f, 0)), TRUNK_R * s, TRUNK_R * 0.8f * s,
			TRUNK_H * s + 0.03f, 10, trunk, &ends);

		const int cols = 28, rows = 6;
		Material needles = palette::Needles();
		needles.color *= 0.9f + 0.2f * Hash(treeIndex + 0.3f);
		Material under = needles;
		under.color *= 0.55f;
		Material snow = palette::Snow();

		for (int k = 0; k < 3; k++)
		{
			const Tier& tier = TIERS[k];
			const float seed = TreeSeed(treeIndex, k);
			auto local = [&](glm::vec3 p) { return glm::vec3(root * glm::vec4(p, 1.0f)); };
			auto theta = [&](int j) { return 2.0f * PI * j / cols; };

			// green cone
			mb.Grid(rows, cols, true,
				[&](int i, int j) { return local(TierPoint(tier, s, seed, theta(j), (float)i / (rows - 1), 0.0f)); },
				[&](int i, int j) { return glm::vec2(theta(j) * tier.radius * s, (float)i / (rows - 1) * tier.height * s); },
				needles);

			// underside: from the hem in to the trunk
			const float underY = (tier.bottom + 0.25f * tier.height) * s;
			mb.Grid(3, cols, true,
				[&](int i, int j) {
					glm::vec3 hem = TierPoint(tier, s, seed, theta(j), 1.0f, 0.0f);
					glm::vec3 c(0.0f, underY, 0.0f);
					return local(glm::mix(hem, c, i / 2.0f));
				},
				[&](int, int) { return glm::vec2(0.0f); }, under);

			// snow cap: covers the top part, wavy lower edge with a rounded lip
			auto capEnd = [&](float th) {
				return 0.46f + 0.13f * std::sin(5.0f * th + seed * 1.7f) + 0.07f * std::sin(11.0f * th + seed);
			};
			const int capRows = 8;
			mb.Grid(capRows, cols, true,
				[&](int i, int j) {
					float th = theta(j), e = capEnd(th);
					if (i <= 5) return local(TierPoint(tier, s, seed, th, e * i / 5.0f, 0.004f * s));
					if (i == 6) return local(TierPoint(tier, s, seed, th, e + 0.03f, 0.010f * s));
					return local(TierPoint(tier, s, seed, th, e + 0.05f, 0.001f * s));
				},
				[&](int i, int j) { return glm::vec2((float)j, (float)i); }, snow);
		}
		// a puff of snow on the very top
		glm::vec3 tip = TierPoint(TIERS[2], s, 0.0f, 0.0f, 0.0f, 0.0f);
		mb.Sphere(glm::scale(glm::translate(root, tip + glm::vec3(0, 0.002f, 0)), glm::vec3(1.0f, 0.8f, 1.0f)),
			0.011f * s, 6, 10, snow);
	}
}

void BuildTrees(MeshBuilder& mb)
{
	int index = 0;
	for (const TreeSpot& t : TREES)
		BuildTree(mb, t, index++);
}

// ---------------------------------------------------------------
// Fire pit: ash, stone ring, crossed + leaning logs, glowing coals (flame is in Campfire.cpp)
// ---------------------------------------------------------------
void BuildFirePit(MeshBuilder& mb, InteriorAnchors& anchors)
{
	const float gy = GroundY(FIRE_X, FIRE_Z);
	const glm::mat4 root = At(FIRE_X, gy, FIRE_Z);
	anchors.fire = glm::vec3(FIRE_X, gy, FIRE_Z);

	Material ash = palette::Stone({ 0.20f, 0.19f, 0.19f });
	ash.patternAmount = 0.6f;
	mb.Cylinder(glm::translate(root, glm::vec3(0, -0.004f, 0)), 0.060f, 0.056f, 0.010f, 24, ash, &ash);

	// stone ring: low-poly (faceted) squashed spheres
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

	// leaning logs (tipi)
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

// ---------------------------------------------------------------
// Seats: the log the character sits on, and a stump with a tin mug
// ---------------------------------------------------------------
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
