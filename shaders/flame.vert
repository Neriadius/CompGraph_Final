#version 330 core

// Campfire flame (Person 3): a few teardrop "tongues" that sway and stretch in time.
// The mesh is built once (src/Campfire.cpp); all motion happens here.

layout (location = 0) in vec3 aPos;      // fire space, tongue at rest
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aFlame;    // x = height along the tongue 0..1, y = seed, zw = tongue axis (x, z)

uniform mat4 camMatrix;
uniform mat4 model;                      // fire space -> world
uniform float time;
uniform float fireLevel;                 // 0 = out, 1 = burning (F)

out vec3 vWorldPos;
out vec3 vNormal;
out float vT;
out float vSeed;

void main()
{
	float t = aFlame.x;
	float seed = aFlame.y;
	vec2 axis = aFlame.zw;

	// each tongue stretches with its own rhythm (two sines at unrelated frequencies)
	float stretch = 0.82 + 0.16 * sin(time * 8.3 + seed * 6.1) + 0.08 * sin(time * 21.7 + seed * 2.3);
	float level = fireLevel * (0.92 + 0.08 * sin(time * 3.1 + seed));

	vec3 p = aPos;
	p.xz = axis + (p.xz - axis) * level;                 // thinner as the fire dies
	p.y *= stretch * level;
	p.xz *= mix(1.0, level, 0.5);                        // tongues gather to the centre when small

	// sway: the higher the point, the more it moves
	float sway = t * t * 0.018;
	p.x += sway * sin(time * 6.1 + seed * 11.0 + t * 3.0);
	p.z += sway * cos(time * 5.3 + seed * 5.0 + t * 2.5);

	vec4 world = model * vec4(p, 1.0);
	vWorldPos = world.xyz;
	vNormal = mat3(model) * aNormal;
	vT = t;
	vSeed = seed;
	gl_Position = camMatrix * world;
}
