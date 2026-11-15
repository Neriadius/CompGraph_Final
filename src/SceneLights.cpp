#include "SceneLights.h"

#include <glm/gtc/type_ptr.hpp>

void UploadSceneLights(GLuint program, const SceneLights& lights)
{
	glUniform3fv(glGetUniformLocation(program, "moonDir"), 1, glm::value_ptr(lights.moonDir));
	glUniform3fv(glGetUniformLocation(program, "moonColor"), 1, glm::value_ptr(lights.moonColor));
	glUniform3fv(glGetUniformLocation(program, "skyAmbient"), 1, glm::value_ptr(lights.skyAmbient));
	glUniform3fv(glGetUniformLocation(program, "groundAmbient"), 1, glm::value_ptr(lights.groundAmbient));

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
