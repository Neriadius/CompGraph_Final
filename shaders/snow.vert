#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in float size;
layout (location = 2) in float phase;

uniform mat4 camMatrix;
uniform mat4 model;
uniform float pointScale;
uniform float minPointSize;
uniform float maxPointSize;
uniform bool perspective;
uniform float orthoPointSize;
uniform vec3 camPos;
uniform float nearEnd;
uniform float nearStart;
uniform float farStart;
uniform float farEnd;
uniform float farAlpha;
uniform float time;
uniform float twinkleAmplitude;
uniform float twinkleSpeed;

out float flakeAlpha;

void main()
{
	vec4 worldPos = model * vec4(pos, 1.0);
	gl_Position = camMatrix * worldPos;
	float pixelSize = perspective ? size * pointScale / max(gl_Position.w, 0.0001) : orthoPointSize;
	gl_PointSize = clamp(pixelSize, minPointSize, maxPointSize);
	float dist = length(camPos - worldPos.xyz);
	float nearFade = smoothstep(nearEnd, nearStart, dist);
	float farFade = mix(1.0, farAlpha, smoothstep(farStart, farEnd, dist));
	float twinkle = 1.0 + twinkleAmplitude * sin(time * twinkleSpeed + phase);
	flakeAlpha = nearFade * farFade * twinkle;
}
