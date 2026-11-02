#version 330 core

in vec3 vNormal;
out vec4 FragColor;

void main()
{
	// Placeholder shading so the camera movement is readable.
	// Replace with glass / wood / snow shaders later.
	vec3 n = normalize(vNormal);
	float l = max(dot(n, normalize(vec3(0.4, 1.0, 0.6))), 0.0);
	FragColor = vec4(vec3(0.25 + 0.75 * l), 1.0);
}
