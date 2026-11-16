#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform vec3 glowColor;                  // colour * intensity

void main()
{
	float r2 = dot(vUV, vUV);
	if (r2 > 1.0) discard;
	float a = exp(-r2 * 4.5) * (1.0 - r2);    // gaussian that reaches exactly 0 at the edge
	FragColor = vec4(glowColor * a, 1.0);     // additive blending (ONE, ONE)
}
