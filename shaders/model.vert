#version 330 core

// Imported OBJ models: hut, trees, snowman, Typhoeus. Fragment shader is the
// same as the procedural interior (interior.frag), so the lighting is identical

// Morph animation: every vertex stores two poses, morph blends between them
// (0 = first pose, 1 = second)

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec3 aPos2;
layout (location = 4) in vec3 aNormal2;

uniform mat4 camMatrix;
uniform mat4 model;
uniform mat3 normalMatrix;
uniform float morph;

// per material (set by src/Model.cpp)
uniform vec3 baseColor;
uniform vec4 material;                   // specular, shininess, pattern id, pattern amount
uniform vec2 emission;                   // window-light weight, fire-light weight

out vec3 vWorldPos;
out vec3 vLocalPos;
out vec3 vNormal;
out vec3 vColor;
out vec2 vUV;
flat out vec4 vMat;
flat out vec2 vEmit;

void main()
{
	vec3 p = mix(aPos, aPos2, morph);
	vec3 n = mix(aNormal, aNormal2, morph);

	vec4 world = model * vec4(p, 1.0);
	vWorldPos = world.xyz;
	vLocalPos = p;
	vNormal = normalMatrix * n;
	vColor = baseColor;
	vUV = aUV;
	vMat = material;
	vEmit = emission;
	gl_Position = camMatrix * world;
}
