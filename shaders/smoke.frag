#version 330 core

in vec2 vUV;
in float vAlpha;
in float vSeed;
out vec4 FragColor;

uniform vec3 smokeColor;                 // moonlit grey-blue

float hash12(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}

float valueNoise2(vec2 p)
{
	vec2 i = floor(p), f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash12(i), hash12(i + vec2(1, 0)), f.x),
	           mix(hash12(i + vec2(0, 1)), hash12(i + vec2(1, 1)), f.x), f.y);
}

void main()
{
	float r = length(vUV);
	float n = valueNoise2(vUV * 2.5 + vSeed * 7.3);
	float a = vAlpha * 0.45 * smoothstep(1.0, 0.25, r + 0.35 * (n - 0.5));
	if (a <= 0.003) discard;
	vec3 col = smokeColor * (0.85 + 0.3 * n);
	FragColor = vec4(col * a, a);            // premultiplied: blend ONE, ONE_MINUS_SRC_ALPHA
}
