#include "SceneLights.h"

#include <glm/gtc/type_ptr.hpp>

namespace
{
	// 0 at night and day, 1 half way: used for the dawn tint
	float Dawn(float d) { return 4.0f * d * (1.0f - d); }
}

void ApplyDaylight(SceneLights& lights, float d)
{
	lights.daylight = d;
	// the key light swings from the moon to the sun (a sunrise when d goes 0 -> 1)
	lights.moonDir = glm::normalize(glm::mix(sky::MOON_DIR, sky::SUN_DIR, d) + glm::vec3(0.0f, -0.25f * Dawn(d), 0.0f));
	lights.moonColor = glm::mix(sky::MOON_COLOR, sky::SUN_COLOR, d) + sky::DAWN * (0.45f * Dawn(d));
	lights.skyAmbient = glm::mix(sky::NIGHT_SKY, sky::DAY_SKY, d);
	lights.groundAmbient = glm::mix(sky::NIGHT_GROUND, sky::DAY_GROUND, d);
}

glm::vec3 BackgroundColor(float d)
{
	return glm::mix(sky::NIGHT_BACKGROUND, sky::DAY_BACKGROUND, d) + sky::DAWN * (0.25f * Dawn(d));
}

OutsideLightValues OutsideLight(float d)
{
	OutsideLightValues night{ glm::vec3(0.4f, 1.0f, 0.6f), glm::vec3(0.75f, 0.70f, 0.62f), glm::vec3(0.35f, 0.37f, 0.45f) };
	OutsideLightValues day{ sky::SUN_DIR, glm::vec3(0.95f, 0.90f, 0.82f), glm::vec3(0.55f, 0.58f, 0.66f) };
	return { glm::mix(night.dir, day.dir, d), glm::mix(night.color, day.color, d) + sky::DAWN * (0.25f * Dawn(d)),
		glm::mix(night.ambient, day.ambient, d) };
}

void UploadSceneLights(GLuint program, const SceneLights& lights)
{
	glUniform3fv(glGetUniformLocation(program, "moonDir"), 1, glm::value_ptr(lights.moonDir));
	glUniform3fv(glGetUniformLocation(program, "moonColor"), 1, glm::value_ptr(lights.moonColor));
	glUniform3fv(glGetUniformLocation(program, "skyAmbient"), 1, glm::value_ptr(lights.skyAmbient));
	glUniform3fv(glGetUniformLocation(program, "groundAmbient"), 1, glm::value_ptr(lights.groundAmbient));
	glUniform1f(glGetUniformLocation(program, "daylight"), lights.daylight);

	glm::vec3 pos[MAX_POINT_LIGHTS], col[MAX_POINT_LIGHTS];
	glm::vec2 falloff[MAX_POINT_LIGHTS];
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		pos[i] = lights.points[i].position;
		col[i] = lights.points[i].color;
		falloff[i] = glm::vec2(lights.points[i].softRadius, lights.points[i].range);
	}
	glUniform1i(glGetUniformLocation(program, "pointCount"), lights.pointCount);
	glUniform3fv(glGetUniformLocation(program, "pointPos"), MAX_POINT_LIGHTS, glm::value_ptr(pos[0]));
	glUniform3fv(glGetUniformLocation(program, "pointColor"), MAX_POINT_LIGHTS, glm::value_ptr(col[0]));
	glUniform2fv(glGetUniformLocation(program, "pointFalloff"), MAX_POINT_LIGHTS, glm::value_ptr(falloff[0]));
}
