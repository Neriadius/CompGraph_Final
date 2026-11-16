#ifndef INTERIOR_LAYOUT_H
#define INTERIOR_LAYOUT_H

// Terrain: snowy mountains around the back of the globe, a flat meadow in front where the hut, campfire and character stand

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "ModelHeights.h"

namespace layout
{
	constexpr float PI = 3.14159265358979f;
	constexpr float DEG = PI / 180.0f;

	// ground
	constexpr float GROUND_DISC_R = 0.826f;      // just inside the glass at ground level
	constexpr float REST_LIFT = 0.008f;          // flakes rest this much above a visible surface

	// small snow drifts on the meadow
	struct Drift { float x, z, radius, height; };
	inline constexpr Drift DRIFTS[] = {
		{ 0.48f, 0.12f, 0.16f, 0.028f },
		{ -0.52f, 0.20f, 0.16f, 0.030f },
		{ -0.10f, 0.62f, 0.15f, 0.024f },
		{ 0.38f, 0.55f, 0.13f, 0.020f },
	};

	// mountains: gaussian peaks, made craggy by ridged noise (Mountains())
	struct Peak { float x, z, height, radius; };
	inline constexpr Peak PEAKS[] = {
		{ 0.02f, -0.60f, 0.70f, 0.30f },      // main summit behind the hut
		{ -0.44f, -0.50f, 0.50f, 0.25f },
		{ 0.44f, -0.50f, 0.52f, 0.25f },
		{ -0.20f, -0.72f, 0.42f, 0.18f },
		{ 0.26f, -0.70f, 0.40f, 0.18f },
		{ -0.67f, -0.10f, 0.30f, 0.19f },     // side ridges
		{ 0.67f, -0.16f, 0.32f, 0.19f },
		{ -0.62f, 0.44f, 0.11f, 0.16f },      // low hills in front
		{ 0.60f, 0.46f, 0.10f, 0.15f },
	};

	// hut (models/hut/hut.obj). Local frame: door on +Z (front), window on +X
	constexpr float HUT_X = -0.26f, HUT_Z = -0.10f;
	constexpr float HUT_YAW = 38.0f * DEG;        // turns the door towards the campfire
	constexpr float HUT_SINK = 0.012f;            // floor level with the snow
	// outer faces of the log walls, hut-local
	constexpr float HUT_FRONT = 0.186f, HUT_BACK = -0.183f, HUT_LEFT = -0.197f, HUT_RIGHT = 0.184f;
	constexpr float DOOR_LX = -0.010f;            // door centre along the front wall
	// points measured on the model: outside the window, top of the chimney (hut-local, y from the base)
	inline constexpr float WINDOW_LIGHT[3] = { 0.209f, 0.154f, 0.0f };
	inline constexpr float CHIMNEY_TOP[3] = { -0.010f, 0.392f, -0.113f };

	// campfire, seat, Typhoeus
	constexpr float FIRE_X = 0.10f, FIRE_Z = 0.24f;
	constexpr float CHAR_X = 0.185f, CHAR_Z = 0.11f;                // seat log centre
	constexpr float CHAR_YAW = -32.0f * DEG;                        // faces the fire
	constexpr float STUMP_X = -0.05f, STUMP_Z = 0.30f;              // second seat with a mug
	constexpr float SNOWMAN_X = -0.40f, SNOWMAN_Z = 0.42f, SNOWMAN_YAW = 30.0f * DEG;

	struct FlatSpot { float x, z, radius; };
	inline constexpr FlatSpot FLAT[] = {
		{ HUT_X, HUT_Z, 0.38f },
		{ FIRE_X, FIRE_Z, 0.24f },
		{ SNOWMAN_X, SNOWMAN_Z, 0.10f },
	};

	// trees (models/trees): snow rests on the pines and shrubs
	enum TreeModel { PINE = 0, PINE_TALL = 1, SHRUB = 2 };
	struct TreeSpot { float x, z, scale, yawDeg; int model; };
	inline constexpr TreeSpot TREES[] = {
		{ 0.47f, -0.10f, 1.00f, 0.0f, PINE_TALL },     // meadow edge
		{ -0.60f, 0.16f, 0.95f, 15.0f, PINE_TALL },
		{ 0.58f, 0.26f, 0.75f, 70.0f, PINE },
		{ -0.28f, 0.58f, 0.62f, 110.0f, PINE },
		{ 0.38f, 0.56f, 0.55f, 200.0f, PINE },
		{ 0.24f, -0.34f, 0.80f, 40.0f, PINE },         // on the mountain slopes
		{ -0.05f, -0.40f, 0.62f, 300.0f, PINE },
		{ 0.62f, -0.36f, 0.60f, 150.0f, PINE },
		{ -0.62f, -0.36f, 0.66f, 230.0f, PINE },
		{ 0.30f, 0.06f, 0.95f, 30.0f, SHRUB },
		{ 0.04f, 0.68f, 1.10f, 120.0f, SHRUB },
		{ -0.52f, -0.22f, 1.00f, 60.0f, SHRUB },
	};
	struct BushSpot { float x, z, scale, yawDeg; };
	inline constexpr BushSpot BUSHES[] = {
		{ 0.70f, 0.08f, 0.9f, 20.0f },
		{ -0.70f, 0.32f, 1.0f, 140.0f },
		{ 0.22f, 0.68f, 0.7f, 250.0f },
	};

	// Height functions (offsets above GROUND_Y)
	inline float Smooth(float e0, float e1, float x)
	{
		float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	// cos / sin of the hut's yaw
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

	// Noise for the mountain crags (integer hash, same result on every compiler)
	inline float Hash2(int x, int z)
	{
		uint32_t h = (uint32_t)x * 374761393u + (uint32_t)z * 668265263u;
		h = (h ^ (h >> 13)) * 1274126177u;
		return (float)((h ^ (h >> 16)) & 0xffffffu) / 16777215.0f;
	}

	inline float ValueNoise(float x, float z)
	{
		float fx = std::floor(x), fz = std::floor(z);
		int ix = (int)fx, iz = (int)fz;
		float tx = x - fx, tz = z - fz;
		tx = tx * tx * (3.0f - 2.0f * tx);
		tz = tz * tz * (3.0f - 2.0f * tz);
		float a = Hash2(ix, iz), b = Hash2(ix + 1, iz), c = Hash2(ix, iz + 1), d = Hash2(ix + 1, iz + 1);
		return (a + (b - a) * tx) + ((c + (d - c) * tx) - (a + (b - a) * tx)) * tz;
	}

	// Ridged multifractal: sharp crests where the noise crosses 0.5
	inline float Ridged(float x, float z)
	{
		float sum = 0.0f, amp = 0.55f, norm = 0.0f;
		for (int o = 0; o < 5; o++)
		{
			float n = 1.0f - std::fabs(2.0f * ValueNoise(x, z) - 1.0f);
			sum += n * n * amp;
			norm += amp;
			x = x * 2.03f + 17.1f;
			z = z * 2.03f - 9.3f;
			amp *= 0.5f;
		}
		return sum / norm;
	}

	// Height of the ground before the cache: mountains + drifts, flattened where objects stand
	inline float TerrainRaw(float x, float z)
	{
		float mountains = 0.0f;
		for (const Peak& p : PEAKS)
		{
			float dx = x - p.x, dz = z - p.z;
			mountains += p.height * std::exp(-1.6f * (dx * dx + dz * dz) / (p.radius * p.radius));
		}
		// crags: warp the coordinates a little so the ridges are not straight
		float wx = x + 0.12f * (ValueNoise(z * 3.0f + 5.0f, x * 3.0f) - 0.5f);
		float wz = z + 0.12f * (ValueNoise(x * 3.0f - 7.0f, z * 3.0f) - 0.5f);
		mountains *= 0.45f + 0.85f * Ridged(wx * 3.4f, wz * 3.4f);

		float drifts = 0.0f;
		for (const Drift& d : DRIFTS)
		{
			float dx = x - d.x, dz = z - d.z;
			drifts += d.height * std::exp(-(dx * dx + dz * dz) / (d.radius * d.radius));
		}

		float flat = 0.0f;                       // 1 = must be flat
		for (const FlatSpot& f : FLAT)
			flat = std::max(flat, 1.0f - Smooth(f.radius, f.radius * 1.45f, std::hypot(x - f.x, z - f.z)));
		float rim = Smooth(GROUND_DISC_R, 0.68f, std::sqrt(x * x + z * z));
		return (mountains + drifts) * (1.0f - flat) * rim;
	}

	// The height map: GRID x GRID samples over the ground disc
	constexpr int TERRAIN_GRID = 200;
	constexpr float TERRAIN_HALF = 0.84f;
	inline const std::vector<float>& TerrainField()
	{
		static std::vector<float> field;
		if (field.empty())
		{
			field.resize(TERRAIN_GRID * TERRAIN_GRID);
			const float cell = 2.0f * TERRAIN_HALF / (TERRAIN_GRID - 1);
			for (int iz = 0; iz < TERRAIN_GRID; iz++)
				for (int ix = 0; ix < TERRAIN_GRID; ix++)
					field[iz * TERRAIN_GRID + ix] = TerrainRaw(-TERRAIN_HALF + ix * cell, -TERRAIN_HALF + iz * cell);
		}
		return field;
	}

	// Ground height offset at (x, z): bilinear sample of the height map
	inline float TerrainOffset(float x, float z)
	{
		const std::vector<float>& f = TerrainField();
		const float cell = 2.0f * TERRAIN_HALF / (TERRAIN_GRID - 1);
		float gx = std::clamp((x + TERRAIN_HALF) / cell, 0.0f, TERRAIN_GRID - 1.001f);
		float gz = std::clamp((z + TERRAIN_HALF) / cell, 0.0f, TERRAIN_GRID - 1.001f);
		int ix = (int)gx, iz = (int)gz;
		float tx = gx - ix, tz = gz - iz;
		const float* r0 = &f[iz * TERRAIN_GRID + ix];
		const float* r1 = r0 + TERRAIN_GRID;
		return (r0[0] * (1 - tx) + r0[1] * tx) * (1 - tz) + (r1[0] * (1 - tx) + r1[1] * tx) * tz;
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
