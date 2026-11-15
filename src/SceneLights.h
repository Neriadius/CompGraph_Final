#ifndef SCENE_LIGHTS_H
#define SCENE_LIGHTS_H

// Lights of the globe interior, handed from the interior scene (Person 3) to any shader
// (Person 4: glass, base, key...). Everything is in WORLD space and already includes the
// shake, so a shader only has to read the uniforms below.
//
// Use it from any shader program:
//     const SceneLights& lights = scene.Lights();     // updated every frame by Scene::Update
//     myShader->Activate();
//     UploadSceneLights(myShader->ID, lights);
//
// GLSL side (copy into the fragment shader; see shaders/interior.frag for a full example):
//     #define MAX_POINT_LIGHTS 6
//     uniform vec3  moonDir;                       // direction TOWARDS the moon
//     uniform vec3  moonColor;
//     uniform vec3  skyAmbient, groundAmbient;     // hemisphere ambient (up / down)
//     uniform int   pointCount;
//     uniform vec3  pointPos[MAX_POINT_LIGHTS];
//     uniform vec3  pointColor[MAX_POINT_LIGHTS];  // colour * intensity (already flickering)
//     uniform vec2  pointFalloff[MAX_POINT_LIGHTS];// x = soft radius r0, y = range (light is 0 beyond)
//
//   attenuation = 1 / (1 + (d / r0)^2) * window,  window = clamp(1 - (d / range)^4, 0, 1)^2
//   interior.frag also adds a weak fill: skyAmbient * 0.8 * max(dot(N, F), 0),
//   F = normalize(vec3(-moonDir.x, 0.35, -moonDir.z)) (sky light from the side opposite the moon)

#include <glad/glad.h>
#include <glm/glm.hpp>

constexpr int MAX_POINT_LIGHTS = 6;

struct PointLight
{
	glm::vec3 position = glm::vec3(0.0f);   // world space
	glm::vec3 color = glm::vec3(0.0f);      // colour * current intensity (0 = off)
	float softRadius = 0.1f;                // r0: distance where the light has half strength
	float range = 0.5f;                     // no light beyond this distance
};

struct SceneLights
{
	glm::vec3 moonDir = glm::normalize(glm::vec3(-0.4f, 0.8f, 0.5f));   // same as the glass highlight
	glm::vec3 moonColor = glm::vec3(0.55f, 0.60f, 0.74f);
	glm::vec3 skyAmbient = glm::vec3(0.21f, 0.25f, 0.38f);
	glm::vec3 groundAmbient = glm::vec3(0.18f, 0.19f, 0.25f);   // moonlight bounced off the snow

	int pointCount = 0;
	PointLight points[MAX_POINT_LIGHTS];

	// Named slots, so other shaders can pick only the light they need
	static const int FIRE = 0;              // campfire, flickers (F toggles)
	static const int FIRST_WINDOW = 1;      // hut windows + lantern (L toggles)
};

// Uploads all light uniforms to the active program (missing uniforms are ignored by GL).
void UploadSceneLights(GLuint program, const SceneLights& lights);

#endif
