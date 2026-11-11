#version 330 core

in vec3 worldPos;
in vec3 normal;
out vec4 FragColor;

uniform vec3 camPos;
uniform vec3 lightDir;   // normalized direction TOWARD the light
uniform vec3 glassTint;
uniform float glassAlpha;

void main()
{
	vec3 N = normalize(normal);
	if (!gl_FrontFacing) N = -N;   // inner surface
	vec3 V = normalize(camPos - worldPos);
	vec3 H = normalize(lightDir + V);
	float spec = pow(max(dot(N, H), 0.0), 96.0) * 1.2;
	float fres = pow(1.0 - max(dot(N, V), 0.0), 3.0);
	float alpha = clamp(glassAlpha + 0.5 * fres, 0.0, 1.0);
	// Premultiplied output: keep specular highlights independent of alpha.
	vec3 rgb = glassTint * alpha + vec3(spec);
	FragColor = vec4(rgb, alpha);
}
