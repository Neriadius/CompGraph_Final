#ifndef SCENE_CONSTANTS_H
#define SCENE_CONSTANTS_H

#include "InteriorLayout.h"

constexpr float GROUND_Y_RATIO = -0.55f;

// Where snow comes to rest: the ground level plus the snow drifts, the hut roof and the
// pine branches (InteriorLayout.h). Flat ground still returns GROUND_Y_RATIO.
inline float SurfaceHeightAt(float x, float z, float globeRadius)
{
	return (GROUND_Y_RATIO + layout::SurfaceOffset(x / globeRadius, z / globeRadius)) * globeRadius;
}

#endif
