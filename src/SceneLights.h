#ifndef SCENE_LIGHTS_H
#define SCENE_LIGHTS_H

// Lights of the globe interior

// Day / night (I key): "moonDir / moonColor" is the KEY light: the moon at night, the sun by day
// ApplyDaylight() blends all values; `daylight` goes 0 (night) -> 1 (day) over a few
// seconds, with a warm dawn tint half way

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

// Night and day settings
namespace sky
{
	const glm::vec3 MOON_DIR = glm::normalize(glm::vec3(-0.4f, 0.8f, 0.5f));   // same as the glass highlight
	const glm::vec3 MOON_COLOR(0.55f, 0.60f, 0.74f);
	const glm::vec3 NIGHT_SKY(0.21f, 0.25f, 0.38f);
	const glm::vec3 NIGHT_GROUND(0.18f, 0.19f, 0.25f);      // moonlight bounced off the snow
	const glm::vec3 NIGHT_BACKGROUND(0.05f, 0.06f, 0.10f);

	const glm::vec3 SUN_DIR = glm::normalize(glm::vec3(0.55f, 0.62f, 0.38f));
	const glm::vec3 SUN_COLOR(0.88f, 0.83f, 0.74f);
	const glm::vec3 DAY_SKY(0.30f, 0.36f, 0.47f);
	const glm::vec3 DAY_GROUND(0.26f, 0.27f, 0.31f);        // bright snow bounce
	const glm::vec3 DAY_BACKGROUND(0.50f, 0.68f, 0.88f);

	const glm::vec3 DAWN(1.0f, 0.55f, 0.30f);               // tint half way through the change
}

struct SceneLights
{
	glm::vec3 moonDir = sky::MOON_DIR;          // key light direction (moon / sun)
	glm::vec3 moonColor = sky::MOON_COLOR;      // key light colour
	glm::vec3 skyAmbient = sky::NIGHT_SKY;
	glm::vec3 groundAmbient = sky::NIGHT_GROUND;
	float daylight = 0.0f;                      // 0 = night, 1 = day

	int pointCount = 0;
	PointLight points[MAX_POINT_LIGHTS];

	// Named slots, so other shaders can pick only the light they need
	static const int FIRE = 0;              // campfire, flickers (F toggles)
	static const int FIRST_WINDOW = 1;      // hut windows + lantern (L toggles)
};

// Uploads all light uniforms to the active program (missing uniforms are ignored by GL)
void UploadSceneLights(GLuint program, const SceneLights& lights);

// Sets key light, ambient and daylight for a day amount 0..1 (point lights are not touched)
void ApplyDaylight(SceneLights& lights, float daylight);

// Background (clear) colour for a day amount 0..1
glm::vec3 BackgroundColor(float daylight);

// One directional light for simple shaders (wooden base, brass key). At night it gives the
// values those shaders used before, so they look the same as without day / night
struct OutsideLightValues { glm::vec3 dir, color, ambient; };
OutsideLightValues OutsideLight(float daylight);

#endif
