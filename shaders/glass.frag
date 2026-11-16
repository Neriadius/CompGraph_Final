#version 330 core

in vec3 worldPos;
in vec3 normal;
out vec4 FragColor;

uniform vec3 camPos;
uniform vec3 lightDir;   // normalized direction TOWARD the light
uniform vec3 glassTint;
uniform float glassAlpha;
uniform samplerCube skyBox;
uniform float reflectStrength;
uniform float daylight;  // 0 = night sky in the reflection, 1 = day sky (I key)

void main()
{
	vec3 N = normalize(normal);
	if (!gl_FrontFacing) N = -N;   // inner surface
	vec3 V = normalize(camPos - worldPos);
	vec3 H = normalize(lightDir + V);
	float spec = pow(max(dot(N, H), 0.0), 96.0) * 1.2;
	float fres = pow(1.0 - max(dot(N, V), 0.0), 3.0);
	float alpha = clamp(glassAlpha + 0.5 * fres, 0.0, 1.0);
	vec3 R = reflect(-V, N);
	vec3 daySky = R.y > 0.0 ? mix(vec3(0.42, 0.48, 0.55), vec3(0.22, 0.34, 0.52), R.y) : vec3(0.38, 0.40, 0.44);
	vec3 refl = mix(texture(skyBox, R).rgb, daySky, daylight);
	float k = reflectStrength * (0.25 + 0.75 * fres);
	alpha = clamp(alpha + 0.3 * k, 0.0, 1.0);
	// Premultiplied output: keep specular highlights independent of alpha.
	vec3 rgb = glassTint * alpha + refl * k + vec3(spec);
	FragColor = vec4(rgb, alpha);
}
