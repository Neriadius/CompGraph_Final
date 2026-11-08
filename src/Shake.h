#ifndef SHAKE_H
#define SHAKE_H

#include <cmath>

struct Shake
{
	float intensity = 0.0f;
	float decay = 1.5f;

	void Impulse(float amount)
	{
		intensity += amount;
		if (intensity > 1.0f) intensity = 1.0f;
	}

	void Update(float dt)
	{
		intensity *= std::exp(-decay * dt);
		if (intensity < 0.001f) intensity = 0.0f;
	}
};

#endif
