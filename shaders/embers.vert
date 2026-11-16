#version 330 core

// Sparks rising from the campfire. Every spark's path is computed from gl_VertexID and time.

uniform mat4 camMatrix;
uniform mat4 model;                      // fire space -> world
uniform float time;
uniform float fireLevel;
uniform float pointScale;                // viewport height / (2 tan(fov / 2))

out float vLife;                         // 1 = just born, 0 = burnt out

float hash(float n) { return fract(sin(n * 13) * 43758); }

void main()
{
	float id = float(gl_VertexID);
	float period = 1.4 + 1.6 * hash(id + 0.5);
	float age = fract(time / period + hash(id + 7.1));          // 0..1 along the spark's life

	float angle = 6.2831853 * hash(id + 3.3) + age * (1.5 + 2.0 * hash(id + 4.4));
	float radius = 0.008 + 0.025 * hash(id + 9.7) + age * 0.03;
	float height = 0.03 + age * (0.20 + 0.12 * hash(id + 1.9));
	vec3 p = vec3(cos(angle) * radius, height, sin(angle) * radius);
	p.x += 0.012 * age * sin(time * 2.7 + id);

	vec4 clip = camMatrix * model * vec4(p, 1.0);
	gl_Position = clip;

	// fewer sparks while the fire is small; each spark fades as it rises
	float alive = step(hash(id + 2.2), fireLevel);
	vLife = (1.0 - age) * alive;
	float worldSize = 0.0045 * (0.4 + 0.6 * (1.0 - age));
	gl_PointSize = clamp(worldSize * pointScale / max(clip.w, 1e-3), 1.0, 8.0) * alive;
}
