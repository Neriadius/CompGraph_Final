#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 camMatrix;   // projection * view (uploaded by Camera::Matrix)
uniform mat4 model;       // object -> world (includes the shake and the key turning)

out vec3 vLocalPos;       // position in the key's own space: the pattern turns with the key
out vec3 vLocalNormal;
out vec3 vWorldPos;

void main()
{
	vec4 world = model * vec4(aPos, 1.0);
	vLocalPos = aPos;
	vLocalNormal = aNormal;
	vWorldPos = world.xyz;
	gl_Position = camMatrix * world;
}
