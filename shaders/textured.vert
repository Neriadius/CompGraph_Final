#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 camMatrix;   // projection * view (uploaded by Camera::Matrix)
uniform mat4 model;       // object -> world (includes the shake)

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vUV;

void main()
{
	vNormal = mat3(transpose(inverse(model))) * aNormal;
	vWorldPos = vec3(model * vec4(aPos, 1.0));
	vUV = aUV;
	gl_Position = camMatrix * model * vec4(aPos, 1.0);
}
