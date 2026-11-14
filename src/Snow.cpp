// Tune SNOW_COUNT, sizes, lift, turbulence and agitation below for density and motion.
// Person 3 can extend SurfaceHeightAt in SceneConstants.h for hut and trees.
#define SNOW_DEBUG_START_AIRBORNE 0

#include "Snow.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <GL/freeglut.h>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "SceneConstants.h"
#include "shaderClass.h"

const int SNOW_COUNT = 6000;
const float SNOW_PI = 3.14159265358979f;
const float SNOW_DISC_MARGIN = 0.95f;
const float SNOW_SIZE_MIN = 0.008f, SNOW_SIZE_MAX = 0.018f;
const float SNOW_FALL_MIN = 0.12f, SNOW_FALL_MAX = 0.25f;
const float SNOW_MIN_PX = 2.0f, SNOW_MAX_PX = 24.0f;
const float SNOW_ORTHO_PX = 4.0f;
const float SNOW_NEAR_END = 0.03f, SNOW_NEAR_START = 0.12f;
const float SNOW_FAR_START = 3.0f, SNOW_FAR_END = 6.0f;
const float SNOW_FAR_ALPHA = 0.5f;
const float SNOW_TWINKLE_AMPLITUDE = 0.1f, SNOW_TWINKLE_SPEED = 1.0f;
const float SNOW_SPAWN_RADIUS = 0.9f;
const float SNOW_VERTICAL_RESPONSE = 1.5f;
const float SNOW_SWAY = 0.05f;
const float SNOW_DRAG = 0.8f;
const float SNOW_WALL_RATIO = 0.93f;
const float SNOW_WALL_DAMPING = 0.5f;
const float SNOW_MAX_SPEED = 3.0f;
const float SNOW_AGITATION_DECAY = 0.2f;
const float SNOW_LIFT_RATE = 6.0f;
const float SNOW_LIFT_UP = 1.0f, SNOW_LIFT_RANDOM_UP = 1.5f;
const float SNOW_LIFT_HORIZONTAL = 0.6f;
const float SNOW_FALL_REDUCTION = 0.85f;
const float SNOW_TURBULENCE = 4.0f, SNOW_SWIRL = 1.5f;
const float SNOW_DRAG_REDUCTION = 0.6f;
const float SNOW_FIELD_SCALE = 3.0f;
const unsigned int SNOW_SEED = 42;

void Snow::Init()
{
	shader = new Shader("shaders/snow.vert", "shaders/snow.frag");
	particles.resize(SNOW_COUNT);
	vertices.resize(SNOW_COUNT);
	agitation = 0.0f;
	rng.seed(SNOW_SEED);
	radiusInitialized = false;

	// Init has no frame radius; build a unit disc and scale it in the first Update.
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	float ground = SurfaceHeightAt(0.0f, 0.0f, 1.0f);
	float discRadius = std::sqrt(1.0f - ground * ground) * SNOW_DISC_MARGIN;
	for (Particle& particle : particles)
	{
		float radius = std::sqrt(unit(rng)) * discRadius;
		float angle = unit(rng) * 2.0f * SNOW_PI;
		float x = radius * std::cos(angle), z = radius * std::sin(angle);
		particle.pos = glm::vec3(x, SurfaceHeightAt(x, z, 1.0f), z);
		particle.vel = glm::vec3(0.0f);
		particle.size = SNOW_SIZE_MIN + unit(rng) * (SNOW_SIZE_MAX - SNOW_SIZE_MIN);
		particle.fallSpeed = SNOW_FALL_MIN + unit(rng) * (SNOW_FALL_MAX - SNOW_FALL_MIN);
		particle.phase = unit(rng) * 2.0f * SNOW_PI;
		particle.resting = true;
#if SNOW_DEBUG_START_AIRBORNE
		// Rejection sampling gives a uniform distribution in the ball above ground.
		do
		{
			particle.pos = SNOW_SPAWN_RADIUS * glm::vec3(
				2.0f * unit(rng) - 1.0f, 2.0f * unit(rng) - 1.0f, 2.0f * unit(rng) - 1.0f);
		} while (glm::dot(particle.pos, particle.pos) > SNOW_SPAWN_RADIUS * SNOW_SPAWN_RADIUS ||
			particle.pos.y <= SurfaceHeightAt(particle.pos.x, particle.pos.z, 1.0f));
		particle.resting = false;
#endif
	}

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, SNOW_COUNT * sizeof(SnowVertex), nullptr, GL_STREAM_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SnowVertex), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(SnowVertex), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(SnowVertex), (void*)(4 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Snow::Update(const FrameContext& frame)
{
	if (!radiusInitialized)
	{
		for (Particle& particle : particles)
		{
			particle.pos *= frame.globeRadius;
			if (particle.resting)
				particle.pos.y = SurfaceHeightAt(particle.pos.x, particle.pos.z, frame.globeRadius);
		}
		radiusInitialized = true;
	}

	float dt = frame.dt;
	agitation = std::clamp(std::max(agitation * std::exp(-SNOW_AGITATION_DECAY * dt),
		frame.shakeIntensity), 0.0f, 1.0f);
	float liftProbability = std::clamp(frame.shakeIntensity * SNOW_LIFT_RATE * dt, 0.0f, 1.0f);
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	float verticalBlend = 1.0f - std::exp(-dt / SNOW_VERTICAL_RESPONSE);
	float drag = std::exp(-SNOW_DRAG * (1.0f - SNOW_DRAG_REDUCTION * agitation) * dt);
	float wallRadius = SNOW_WALL_RATIO * frame.globeRadius;
	for (Particle& particle : particles)
	{
		if (particle.resting)
		{
			if (liftProbability <= 0.0f || unit(rng) >= liftProbability) continue;
			float angle = unit(rng) * 2.0f * SNOW_PI;
			float up = (SNOW_LIFT_UP + unit(rng) * SNOW_LIFT_RANDOM_UP) * frame.shakeIntensity;
			float horizontal = SNOW_LIFT_HORIZONTAL * frame.shakeIntensity;
			particle.vel = glm::vec3(std::cos(angle) * horizontal, up, std::sin(angle) * horizontal);
			particle.resting = false;
		}
		float fallSpeed = particle.fallSpeed * (1.0f - SNOW_FALL_REDUCTION * agitation);
		particle.vel.y += (-fallSpeed - particle.vel.y) * verticalBlend;
		particle.vel.x += std::sin(frame.time * 0.7f + particle.phase) * SNOW_SWAY * dt;
		particle.vel.z += std::cos(frame.time * 0.9f + particle.phase * 1.3f) * SNOW_SWAY * dt;
		glm::vec3 field = particle.pos * SNOW_FIELD_SCALE +
			frame.time * glm::vec3(1.3f, 0.9f, 1.1f) + glm::vec3(particle.phase);
		particle.vel += agitation * SNOW_TURBULENCE * dt *
			glm::vec3(std::sin(field.x), std::sin(field.y), std::sin(field.z));
		float radial = glm::length(glm::vec2(particle.pos.x, particle.pos.z));
		if (radial > 0.0001f)
			particle.vel += agitation * SNOW_SWIRL * dt *
				glm::vec3(-particle.pos.z / radial, 0.0f, particle.pos.x / radial);
		particle.vel.x *= drag;
		particle.vel.z *= drag;

		float speed = glm::length(particle.vel);
		if (speed > SNOW_MAX_SPEED) particle.vel *= SNOW_MAX_SPEED / speed;
		particle.pos += particle.vel * dt;

		// Positions are globe-local, so the wall is centred at the local origin.
		float dist = glm::length(particle.pos);
		if (dist > wallRadius && dist > 0.0f)
		{
			glm::vec3 normal = particle.pos / dist;
			particle.pos = normal * wallRadius;
			float outward = glm::dot(particle.vel, normal);
			if (outward > 0.0f) particle.vel -= outward * normal;
			particle.vel *= SNOW_WALL_DAMPING;
		}

		float height = SurfaceHeightAt(particle.pos.x, particle.pos.z, frame.globeRadius);
		if (particle.pos.y <= height)
		{
			// Keep the contact point inside the spherical wall at ground level.
			float maxGroundRadius = std::sqrt(std::max(0.0f, wallRadius * wallRadius - height * height));
			float horizontal = glm::length(glm::vec2(particle.pos.x, particle.pos.z));
			if (horizontal > maxGroundRadius && horizontal > 0.0f)
			{
				particle.pos.x *= maxGroundRadius / horizontal;
				particle.pos.z *= maxGroundRadius / horizontal;
				height = SurfaceHeightAt(particle.pos.x, particle.pos.z, frame.globeRadius);
			}
			particle.pos.y = height;
			particle.vel = glm::vec3(0.0f);
			particle.resting = true;
		}
	}
}

void Snow::Draw(const FrameContext& frame, Camera& camera)
{
	for (size_t i = 0; i < particles.size(); i++)
	{
		const Particle& particle = particles[i];
		vertices[i] = { particle.pos.x, particle.pos.y, particle.pos.z, particle.size, particle.phase };
	}

	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	shader->Activate();
	camera.Matrix(45.0f, frame.globeRadius * 0.01f, frame.globeRadius * 50.0f, *shader, "camMatrix");
	glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(frame.globeModel));
	float pointScale = glutGet(GLUT_WINDOW_HEIGHT) / (2.0f * std::tan(glm::radians(45.0f) / 2.0f));
	glUniform1f(glGetUniformLocation(shader->ID, "pointScale"), pointScale);
	glUniform1f(glGetUniformLocation(shader->ID, "minPointSize"), SNOW_MIN_PX);
	glUniform1f(glGetUniformLocation(shader->ID, "maxPointSize"), SNOW_MAX_PX);
	glUniform1i(glGetUniformLocation(shader->ID, "perspective"), camera.perspective ? 1 : 0);
	glUniform1f(glGetUniformLocation(shader->ID, "orthoPointSize"), SNOW_ORTHO_PX);
	glUniform3fv(glGetUniformLocation(shader->ID, "camPos"), 1, glm::value_ptr(frame.camPos));
	glUniform1f(glGetUniformLocation(shader->ID, "nearEnd"), SNOW_NEAR_END * frame.globeRadius);
	glUniform1f(glGetUniformLocation(shader->ID, "nearStart"), SNOW_NEAR_START * frame.globeRadius);
	glUniform1f(glGetUniformLocation(shader->ID, "farStart"), SNOW_FAR_START * frame.globeRadius);
	glUniform1f(glGetUniformLocation(shader->ID, "farEnd"), SNOW_FAR_END * frame.globeRadius);
	glUniform1f(glGetUniformLocation(shader->ID, "farAlpha"), SNOW_FAR_ALPHA);
	glUniform1f(glGetUniformLocation(shader->ID, "time"), frame.time);
	glUniform1f(glGetUniformLocation(shader->ID, "twinkleAmplitude"), SNOW_TWINKLE_AMPLITUDE);
	glUniform1f(glGetUniformLocation(shader->ID, "twinkleSpeed"), SNOW_TWINKLE_SPEED);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	GLsizeiptr bytes = (GLsizeiptr)(vertices.size() * sizeof(SnowVertex));
	glBufferData(GL_ARRAY_BUFFER, bytes, nullptr, GL_STREAM_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, vertices.data());
	glDrawArrays(GL_POINTS, 0, (GLsizei)vertices.size());

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glDisable(GL_PROGRAM_POINT_SIZE);
}
