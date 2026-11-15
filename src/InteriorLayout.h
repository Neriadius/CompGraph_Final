#ifndef INTERIOR_LAYOUT_H
#define INTERIOR_LAYOUT_H

// Where everything inside the globe stands (Person 3).
// One place for the numbers, used by BOTH the geometry (InteriorModels.cpp) and the
// snow (SurfaceHeightAt in SceneConstants.h), so flakes settle exactly on the drifts,
// the hut roof and the pine branches.
//
// Units: globe radius = 1, origin = globe centre. Heights here are OFFSETS above the
// flat ground level (GROUND_Y_RATIO in SceneConstants.h).

#include <algorithm>
#include <cmath>

namespace layout
{
	constexpr float PI = 3.14159265358979f;
	constexpr float DEG = PI / 180.0f;

	// ---- ground ----
	constexpr float GROUND_DISC_R = 0.826f;      // just inside the glass at ground level
	constexpr float REST_LIFT = 0.008f;          // flakes rest this much above a visible surface

	struct Drift { float x, z, radius, height; };
	inline constexpr Drift DRIFTS[] = {
		{ 0.56f, -0.04f, 0.22f, 0.032f },
		{ -0.62f, 0.02f, 0.20f, 0.036f },
		{ 0.02f, -0.66f, 0.20f, 0.030f },
		{ -0.14f, 0.63f, 0.18f, 0.026f },
		{ 0.45f, 0.50f, 0.16f, 0.022f },
		{ 0.30f, -0.55f, 0.14f, 0.020f },
	};

	// ---- hut (log cabin). Local frame: X = width, Z = depth, door on the +Z (front) side ----
	constexpr float HUT_X = -0.22f, HUT_Z = -0.24f;
	constexpr float HUT_YAW = 35.0f * DEG;        // turns the door towards the campfire
	constexpr float HUT_W = 0.32f;                // wall centre lines, local X
	constexpr float HUT_D = 0.38f;                // wall centre lines, local Z
	constexpr float LOG_R = 0.018f;               // wall log radius
	constexpr int LOG_COURSES = 6;
	constexpr float LOG_STEP = 2.0f * LOG_R * 0.92f;
	constexpr float WALL_H = LOG_R + (LOG_COURSES - 1) * LOG_STEP + LOG_R;
	constexpr float ROOF_RISE = 0.15f;            // ridge height above the wall top
	constexpr float ROOF_SLOPE = ROOF_RISE / (0.5f * HUT_W);
	constexpr float ROOF_EAVE = 0.05f;            // overhang past the side walls (local X)
	constexpr float ROOF_GABLE = 0.04f;           // overhang past front / back (local Z)
	constexpr float ROOF_T = 0.018f;              // roof board thickness
	constexpr float ROOF_SNOW_T = 0.022f;         // snow layer on the roof
	constexpr float HUT_SINK = 0.012f;            // bottom logs sit a little in the snow
	constexpr float DOOR_LX = -0.06f, DOOR_W = 0.075f, DOOR_H = 0.145f;

	// ---- campfire, seat, character ----
	constexpr float FIRE_X = 0.13f, FIRE_Z = 0.16f;
	constexpr float CHAR_X = 0.215f, CHAR_Z = 0.03f;                // seat log centre
	constexpr float CHAR_YAW = -32.0f * DEG;                        // faces the fire
	constexpr float STUMP_X = -0.02f, STUMP_Z = 0.22f;              // second seat with a mug

	// ---- pine trees: tiers of cones (scale 1) ----
	struct Tier { float bottom, height, radius; };
	inline constexpr Tier TIERS[] = {
		{ 0.060f, 0.170f, 0.160f },
		{ 0.150f, 0.150f, 0.125f },
		{ 0.240f, 0.150f, 0.090f },
	};
	constexpr float TRUNK_H = 0.08f, TRUNK_R = 0.022f;

	struct TreeSpot { float x, z, scale, yawDeg; };
	inline constexpr TreeSpot TREES[] = {
		{ 0.50f, -0.30f, 1.00f, 0.0f },
		{ 0.15f, -0.62f, 0.78f, 40.0f },
		{ -0.58f, 0.20f, 0.92f, 15.0f },
		{ 0.58f, 0.22f, 0.62f, 70.0f },
		{ -0.30f, 0.52f, 0.55f, 110.0f },
		{ 0.40f, 0.50f, 0.45f, 200.0f },
	};

	// ---------------------------------------------------------------
	// Height functions (offsets above GROUND_Y)
	// ---------------------------------------------------------------
	inline float Smooth(float e0, float e1, float x)
	{
		float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	// cos / sin of the hut's yaw, computed once
	inline const float HUT_COS = std::cos(HUT_YAW);
	inline const float HUT_SIN = std::sin(HUT_YAW);

	// Hut-local coordinates of a ground point
	inline void ToHut(float x, float z, float& lx, float& lz)
	{
		float dx = x - HUT_X, dz = z - HUT_Z;
		lx = HUT_COS * dx - HUT_SIN * dz;     // inverse of the yaw rotation used for the geometry
		lz = HUT_SIN * dx + HUT_COS * dz;
	}

	// Ground point of hut-local (lx, lz); same rotation as glm::rotate(yaw, Y)
	inline void FromHut(float lx, float lz, float& x, float& z)
	{
		x = HUT_X + HUT_COS * lx + HUT_SIN * lz;
		z = HUT_Z - HUT_SIN * lx + HUT_COS * lz;
	}

	inline float TreeSeed(int tree, int tier) { return 1.7f * tree + 4.3f * tier; }

	// Wavy hem of a pine tier: branch tips stick out (tree-local angle)
	inline float TierWave(float theta, float seed)
	{
		return 1.0f + 0.08f * std::sin(7.0f * theta + seed) + 0.04f * std::sin(13.0f * theta + 2.0f * seed);
	}

	// Snow drifts: gaussian bumps, flat under the hut and around the fire, zero at the rim
	inline float TerrainOffset(float x, float z)
	{
		float h = 0.0f;
		for (const Drift& d : DRIFTS)
		{
			float dx = x - d.x, dz = z - d.z;
			float q = (dx * dx + dz * dz) / (d.radius * d.radius);
			if (q < 9.0f) h += d.height * std::exp(-q);      // beyond 3 radii the bump is < 0.01 %
		}
		if (h <= 0.0f) return 0.0f;
		float r = std::sqrt(x * x + z * z);
		float hut = std::hypot(x - HUT_X, z - HUT_Z);
		float fire = std::hypot(x - FIRE_X, z - FIRE_Z);
		return h * Smooth(GROUND_DISC_R, 0.66f, r) * Smooth(0.30f, 0.42f, hut) * Smooth(0.16f, 0.28f, fire);
	}

	// Height of the roof's bottom (board underside) above the ground, hut-local
	inline float RoofLine(float lx)
	{
		return WALL_H - HUT_SINK + ROOF_SLOPE * (0.5f * HUT_W - std::fabs(lx));
	}

	// Top of the snow on the roof, or -1 outside the roof
	inline float RoofSnowTop(float x, float z)
	{
		float lx, lz;
		ToHut(x, z, lx, lz);
		if (std::fabs(lx) > 0.5f * HUT_W + ROOF_EAVE || std::fabs(lz) > 0.5f * HUT_D + ROOF_GABLE) return -1.0f;
		float cosA = 1.0f / std::sqrt(1.0f + ROOF_SLOPE * ROOF_SLOPE);
		return RoofLine(lx) + (ROOF_T + ROOF_SNOW_T) / cosA;
	}

	constexpr int TREE_COUNT = (int)(sizeof(TREES) / sizeof(TREES[0]));
	constexpr float WAVE_MAX = 1.12f;             // largest TierWave value

	// Ground offset under each tree, computed once
	inline const float* TreeBases()
	{
		static float bases[TREE_COUNT];
		static bool ready = false;
		if (!ready)
		{
			for (int i = 0; i < TREE_COUNT; i++) bases[i] = TerrainOffset(TREES[i].x, TREES[i].z);
			ready = true;
		}
		return bases;
	}

	// Outer surface of the pine cones (snow rests on the branches), or -1 outside all trees
	inline float TreeTop(float x, float z)
	{
		const float* bases = TreeBases();
		float best = -1.0f;
		int treeIndex = 0;
		for (const TreeSpot& t : TREES)
		{
			float dx = x - t.x, dz = z - t.z;
			float reach = TIERS[0].radius * t.scale * WAVE_MAX;
			if (dx * dx + dz * dz > reach * reach) { treeIndex++; continue; }   // far from this tree
			float d = std::sqrt(dx * dx + dz * dz);
			float base = bases[treeIndex];
			float theta = std::atan2(dz, dx) + t.yawDeg * DEG;                // tree-local angle
			int tierIndex = 0;
			for (const Tier& tier : TIERS)
			{
				float R = tier.radius * t.scale * TierWave(theta, TreeSeed(treeIndex, tierIndex));
				tierIndex++;
				if (d > R) continue;
				float y = base + (tier.bottom + tier.height * (1.0f - d / R)) * t.scale;
				best = std::max(best, y);
			}
			treeIndex++;
		}
		return best;
	}

	// Where a snowflake comes to rest (offset above GROUND_Y)
	inline float SurfaceOffset(float x, float z)
	{
		float h = TerrainOffset(x, z);
		float roof = RoofSnowTop(x, z);
		if (roof >= 0.0f) h = std::max(h, roof + REST_LIFT);
		float tree = TreeTop(x, z);
		if (tree >= 0.0f) h = std::max(h, tree + REST_LIFT * 1.5f);
		return h;
	}
}

#endif
