#version 330 core

// Camera-facing quad (4 vertices from gl_VertexID, no buffer) for the soft glow
// around the campfire flame and the lantern.

uniform mat4 camMatrix;
uniform vec3 center;                     // world space
uniform vec3 camRight;
uniform vec3 camUp;
uniform float size;                      // half size in world units

out vec2 vUV;

void main()
{
	vec2 c = vec2((gl_VertexID & 1) == 1 ? 1.0 : -1.0, (gl_VertexID & 2) == 2 ? 1.0 : -1.0);
	vUV = c;
	vec3 p = center + (camRight * c.x + camUp * c.y) * size;
	gl_Position = camMatrix * vec4(p, 1.0);
}
