#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 camMatrix;   // projection * view (uploaded by Camera::Matrix)
uniform mat4 model;       // object -> world

out vec3 vNormal;

void main()
{
	vNormal = mat3(model) * aNormal;
	gl_Position = camMatrix * model * vec4(aPos, 1.0);
}
