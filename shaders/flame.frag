#version 330 core

// Flame colour: hot (white-yellow) where the surface faces the viewer, cooler (orange-red) at the silhouette and towards the tips

in vec3 vWorldPos;
in vec3 vNormal;
in float vT;
in float vSeed;

out vec4 FragColor;

uniform vec3 camPos;
uniform float time;
uniform vec3 fireOrigin;                 // world position of the fire's centre

float hash13(vec3 p3)
{
	p3 = fract(p3 * 0.1031);
	p3 += dot(p3, p3.zyx + 31.32);
	return fract((p3.x + p3.y) * p3.z);
}

float valueNoise3(vec3 p)
{
	vec3 i = floor(p), f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix(hash13(i), hash13(i + vec3(1, 0, 0)), f.x),
	               mix(hash13(i + vec3(0, 1, 0)), hash13(i + vec3(1, 1, 0)), f.x), f.y),
	           mix(mix(hash13(i + vec3(0, 0, 1)), hash13(i + vec3(1, 0, 1)), f.x),
	               mix(hash13(i + vec3(0, 1, 1)), hash13(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

void main()
{
	vec3 q = vWorldPos - fireOrigin;
	float n = valueNoise3(vec3(q.x * 70.0, q.y * 45.0 - time * 5.0, q.z * 70.0) + vSeed * 13.0);

	// ragged tips
	if (vT > 0.45 && n < (vT - 0.45) * 1.5) discard;

	vec3 N = normalize(vNormal);
	vec3 V = normalize(camPos - vWorldPos);
	float facing = abs(dot(N, V));
	float heat = clamp(facing * (1.05 - 0.75 * vT) + 0.15 * (n - 0.5), 0.0, 1.0);

	vec3 red    = vec3(0.85, 0.18, 0.04);
	vec3 orange = vec3(1.00, 0.52, 0.10);
	vec3 yellow = vec3(1.00, 0.88, 0.45);
	vec3 white  = vec3(1.00, 0.98, 0.85);

	vec3 col = mix(red, orange, smoothstep(0.10, 0.40, heat));
	col = mix(col, yellow, smoothstep(0.40, 0.70, heat));
	col = mix(col, white, smoothstep(0.80, 0.98, heat));
	FragColor = vec4(col * 1.15, 1.0);
}
