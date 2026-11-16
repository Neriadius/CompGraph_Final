#version 330 core

// Chimney smoke
// puff = gl_VertexID / 6, corner = gl_VertexID % 6 (two triangles facing the camera)

uniform mat4 camMatrix;
uniform vec3 origin;                     // top of the chimney
uniform vec3 camRight;
uniform vec3 camUp;
uniform float time;
uniform float amount;                    // 0..1, smoke only while the hut is lit
uniform int puffCount;

out vec2 vUV;
out float vAlpha;
out float vSeed;

const vec2 CORNERS[6] = vec2[](vec2(-1, -1), vec2(1, -1), vec2(1, 1), vec2(-1, -1), vec2(1, 1), vec2(-1, 1));

void main()
{
	int puff = gl_VertexID / 6;
	vec2 c = CORNERS[gl_VertexID % 6];
	float id = float(puff);

	const float PERIOD = 5.0;                                    // seconds from chimney to fade evenly spread along the plume
	float age = fract(time / PERIOD + id / float(puffCount));

	vec3 wind = vec3(0.07, 0.0, -0.04) * age * age;              // bends the plume over
	vec3 wobble = 0.012 * age * vec3(sin(time * 0.8 + id * 2.1), 0.0, cos(time * 0.7 + id * 1.3));
	vec3 p = origin + vec3(0.0, age * 0.30, 0.0) + wind + wobble;

	float size = mix(0.010, 0.060, sqrt(age));
	vUV = c;
	vSeed = id;
	vAlpha = amount * smoothstep(0.0, 0.10, age) * (1.0 - age);
	gl_Position = camMatrix * vec4(p + (camRight * c.x + camUp * c.y) * size, 1.0);
}
