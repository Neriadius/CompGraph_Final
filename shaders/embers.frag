#version 330 core

in float vLife;
out vec4 FragColor;

void main()
{
	float r = length(gl_PointCoord - vec2(0.5));
	if (r > 0.5 || vLife <= 0.0) discard;
	float a = (1.0 - smoothstep(0.15, 0.5, r)) * vLife;
	vec3 col = mix(vec3(0.9, 0.2, 0.03), vec3(1.0, 0.8, 0.35), vLife);
	FragColor = vec4(col * a, 1.0);          // additive blending (ONE, ONE)
}
