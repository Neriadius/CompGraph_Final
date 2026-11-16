#version 330 core

out vec4 FragColor;
in float flakeAlpha;

uniform sampler2D flakeTex;

void main()
{
	vec3 texColor = texture(flakeTex, gl_PointCoord).rgb;
	float mask = dot(texColor, vec3(0.299, 0.587, 0.114));
	if (mask < 0.05) discard;
	float alpha = clamp(mask * 0.95 * flakeAlpha, 0.0, 1.0);
	vec3 color = mix(vec3(0.88, 0.92, 1.0), vec3(1.0), mask);
	FragColor = vec4(color, alpha);
}
