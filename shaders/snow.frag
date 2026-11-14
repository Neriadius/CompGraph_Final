#version 330 core

out vec4 FragColor;
in float flakeAlpha;

void main()
{
	float radius = length(gl_PointCoord - vec2(0.5));
	if (radius > 0.5) discard;
	float alpha = 0.9 * (1.0 - smoothstep(0.25, 0.5, radius)) * flakeAlpha;
	FragColor = vec4(0.95, 0.97, 1.0, alpha);
}
