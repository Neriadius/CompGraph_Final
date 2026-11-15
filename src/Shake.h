#ifndef SHAKE_H
#define SHAKE_H

#include <cmath>

struct Shake
{
	float intensity = 0.0f;
	float decay = 1.5f;

	// Number of separate "shakes" so far (used by the music box).
	// A key press or mouse click is one shake; dragging counts one shake
	// each time enough movement has been collected (SHAKE_THRESHOLD).
	int count = 0;
	float pending = 0.0f;
	static constexpr float SHAKE_THRESHOLD = 0.4f;

	void Impulse(float amount)
	{
		intensity += amount;
		if (intensity > 1.0f) intensity = 1.0f;

		pending += amount;
		if (pending >= SHAKE_THRESHOLD)
		{
			count++;
			pending = 0.0f;
		}
	}

	void Update(float dt)
	{
		intensity *= std::exp(-decay * dt);
		if (intensity < 0.001f) intensity = 0.0f;

		pending *= std::exp(-2.0f * dt);     // small movements are forgotten after a moment
	}
};

#endif
