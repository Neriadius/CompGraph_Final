#ifndef INTERIOR_LAYOUT_H
#define INTERIOR_LAYOUT_H

// Where everything inside the globe stands (Person 3).
// One place for the numbers, used by BOTH the drawing (Interior.cpp, InteriorModels.cpp,
// HutProps.cpp) and the snow (SurfaceHeightAt in SceneConstants.h), so flakes settle exactly
// on the drifts, the hut roof and the pine branches.
//
// Units: globe radius = 1, origin = globe centre. Heights here are OFFSETS above the
// flat ground level (GROUND_Y_RATIO in SceneConstants.h). The OBJ models in models/ are
// already in these units (base at y = 0, facing +Z), see tools/convert_static.py.

#include <algorithm>
#include <cmath>

#include "ModelHeights.h"

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

	// ---- hut (models/hut/hut.obj). Local frame: door on +Z (front), window on +X ----
	constexpr float HUT_X = -0.22f, HUT_Z = -0.24f;
	constexpr float HUT_YAW = 35.0f * DEG;        // turns the door towards the campfire
	constexpr float HUT_SINK = 0.012f;            // floor level with the snow
	// outer faces of the log walls, hut-local
	constexpr float HUT_FRONT = 0.186f, HUT_BACK = -0.183f, HUT_LEFT = -0.197f, HUT_RIGHT = 0.184f;
	constexpr float DOOR_LX = -0.010f;            // door centre along the front wall
	// points measured on the model: outside the window, top of the chimney (hut-local, y from the base)
	inline constexpr float WINDOW_LIGHT[3] = { 0.209f, 0.154f, 0.0f };
	inline constexpr float CHIMNEY_TOP[3] = { -0.010f, 0.392f, -0.113f };

	// ---- campfire, seat, Typhoeus ----
	constexpr float FIRE_X = 0.13f, FIRE_Z = 0.16f;
	constexpr float CHAR_X = 0.215f, CHAR_Z = 0.03f;                // seat log centre
	constexpr float CHAR_YAW = -32.0f * DEG;                        // faces the fire
	constexpr float STUMP_X = -0.02f, STUMP_Z = 0.22f;              // second seat with a mug
	constexpr float SNOWMAN_X = -0.44f, SNOWMAN_Z = 0.40f, SNOWMAN_YAW = 30.0f * DEG;

	// ---- trees (models/trees): snow rests on the pines and shrubs, not on the bare bushes ----
	enum TreeModel { PINE = 0, PINE_TALL = 1, SHRUB = 2 };
	struct TreeSpot { float x, z, scale, yawDeg; int model; };
	inline constexpr TreeSpot TREES[] = {
		{ 0.50f, -0.30f, 1.00f, 0.0f, PINE_TALL },
		{ 0.15f, -0.62f, 0.85f, 40.0f, PINE },
		{ -0.58f, 0.20f, 0.95f, 15.0f, PINE_TALL },
		{ 0.58f, 0.22f, 0.75f, 70.0f, PINE },
		{ -0.30f, 0.52f, 0.62f, 110.0f, PINE },
		{ 0.40f, 0.50f, 0.55f, 200.0f, PINE },
		{ 0.34f, -0.08f, 0.95f, 30.0f, SHRUB },
		{ 0.03f, 0.66f, 1.10f, 120.0f, SHRUB },
		{ -0.48f, -0.55f, 1.00f, 60.0f, SHRUB },
	};
	struct BushSpot { float x, z, scale, yawDeg; };
	inline constexpr BushSpot BUSHES[] = {
		{ 0.68f, -0.10f, 0.9f, 20.0f },
		{ -0.70f, -0.05f, 1.0f, 140.0f },
		{ 0.25f, 0.66f, 0.7f, 250.0f },
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
		lx = HUT_COS * dx - HUT_SIN * dz;     // inverse of the yaw rotation used for drawing
		lz = HUT_SIN * dx + HUT_COS * dz;
	}

	// Ground point of hut-local (lx, lz); same rotation as glm::rotate(yaw, Y)
	inline void FromHut(float lx, float lz, float& x, float& z)
	{
		x = HUT_X + HUT_COS * lx + HUT_SIN * lz;
		z = HUT_Z - HUT_SIN * lx + HUT_COS * lz;
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

	// Top of the hut (snowy roof, chimney), or -1 outside it
	inline float HutTop(float x, float z)
	{
		float lx, lz;
		ToHut(x, z, lx, lz);
		float h = heights::Sample(heights::HUT, lx, lz);
		return h >= 0.0f ? h - HUT_SINK : -1.0f;
	}

	inline const heights::Field& TreeField(int model)
	{
		return model == PINE_TALL ? heights::PINE_TALL : (model == SHRUB ? heights::SHRUB : heights::PINE);
	}

	constexpr int TREE_COUNT = (int)(sizeof(TREES) / sizeof(TREES[0]));

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

	// Top of the pine branches under (x, z), or -1 outside all trees
	inline float TreeTop(float x, float z)
	{
		const float* bases = TreeBases();
		float best = -1.0f;
		for (int i = 0; i < TREE_COUNT; i++)
		{
			const TreeSpot& t = TREES[i];
			const heights::Field& f = TreeField(t.model);
			float dx = x - t.x, dz = z - t.z;
			float reach = 0.75f * f.cell * f.nx * t.scale;        // bounding radius of the height map
			if (dx * dx + dz * dz > reach * reach) continue;       // far from this tree
			float c = std::cos(t.yawDeg * DEG), s = std::sin(t.yawDeg * DEG);
			float h = heights::Sample(f, (c * dx - s * dz) / t.scale, (s * dx + c * dz) / t.scale);
			if (h >= 0.0f) best = std::max(best, bases[i] + h * t.scale);
		}
		return best;
	}

	// Where a snowflake comes to rest (offset above GROUND_Y)
	inline float SurfaceOffset(float x, float z)
	{
		float h = TerrainOffset(x, z);
		float roof = HutTop(x, z);
		if (roof >= 0.0f) h = std::max(h, roof + REST_LIFT);
		float tree = TreeTop(x, z);
		if (tree >= 0.0f) h = std::max(h, tree + REST_LIFT * 1.5f);
		return h;
	}
}

#endif
