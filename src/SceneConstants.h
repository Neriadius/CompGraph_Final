#ifndef SCENE_CONSTANTS_H
#define SCENE_CONSTANTS_H

// agreed between Person 2 (snow) and Person 3 (scene): change only together
constexpr float GROUND_Y_RATIO = -0.55f;

// Flat ground for now; Person 3 can extend this for hut roofs and trees.
inline float SurfaceHeightAt(float, float, float globeRadius)
{
	return GROUND_Y_RATIO * globeRadius;
}

#endif
