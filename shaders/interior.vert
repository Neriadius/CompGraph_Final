#version 330 core

// Interior of the globe: ground, trees, hut, campfire stones, character
// Vertex layout is written by src/MeshBuilder

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec2 aUV;
layout (location = 4) in vec4 aMat;      // specular strength, shininess, pattern id, pattern amount
layout (location = 5) in vec2 aEmit;     // window-light weight, fire-light weight

uniform mat4 camMatrix;                  // projection * view
uniform mat4 model;                      // mesh -> world (shake + hierarchy)
uniform mat3 normalMatrix;               // transpose(inverse(mat3(model)))

out vec3 vWorldPos;
out vec3 vLocalPos;                      // mesh space: patterns do not swim when the globe shakes
out vec3 vNormal;
out vec3 vColor;
out vec2 vUV;
flat out vec4 vMat;
flat out vec2 vEmit;

void main()
{
	vec4 world = model * vec4(aPos, 1.0);
	vWorldPos = world.xyz;
	vLocalPos = aPos;
	vNormal = normalMatrix * aNormal;
	vColor = aColor;
	vUV = aUV;
	vMat = aMat;
	vEmit = aEmit;
	gl_Position = camMatrix * world;
}
