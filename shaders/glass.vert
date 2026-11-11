#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 camMatrix;
uniform mat4 model;

out vec3 worldPos;
out vec3 normal;

void main()
{
	worldPos = vec3(model * vec4(aPos, 1.0));
	normal = mat3(model) * aNormal;   // model is translation only
	gl_Position = camMatrix * vec4(worldPos, 1.0);
}
