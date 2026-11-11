#include "Cubemap.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

static uint32_t hashTexel(uint32_t face, uint32_t x, uint32_t y)
{
	uint32_t h = face * 0x9e3779b9u ^ x * 0x85ebca6bu ^ y * 0xc2b2ae35u;
	h ^= h >> 16;
	h *= 0x7feb352du;
	h ^= h >> 15;
	h *= 0x846ca68bu;
	return h ^ (h >> 16);
}

// Person 4 can later replace this body with a loaded image set.
GLuint CreateNightSkyCubemap(int size)
{
	if (size <= 0) return 0;

	GLint previousTexture, previousAlignment;
	glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &previousTexture);
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	std::vector<GLubyte> pixels((size_t)size * size * 3);
	for (int face = 0; face < 6; face++)
	{
		for (int y = 0; y < size; y++)
			for (int x = 0; x < size; x++)
			{
				float s = 2.0f * (x + 0.5f) / size - 1.0f;
				float t = 2.0f * (y + 0.5f) / size - 1.0f;
				glm::vec3 dir;
				switch (face)
				{
				case 0: dir = glm::vec3(1.0f, -t, -s); break;
				case 1: dir = glm::vec3(-1.0f, -t, s); break;
				case 2: dir = glm::vec3(s, 1.0f, t); break;
				case 3: dir = glm::vec3(s, -1.0f, -t); break;
				case 4: dir = glm::vec3(s, -t, 1.0f); break;
				default: dir = glm::vec3(-s, -t, -1.0f); break;
				}
				dir = glm::normalize(dir);
				glm::vec3 colour = glm::mix(glm::vec3(0.02f, 0.03f, 0.07f),
					glm::vec3(0.05f, 0.08f, 0.20f), std::max(dir.y, 0.0f));
				if (dir.y < 0.0f) colour *= 1.0f + 0.8f * dir.y;
				float glow = std::max(1.0f - std::abs(dir.y) / 0.1f, 0.0f);
				colour += glm::vec3(0.025f, 0.012f, 0.004f) * glow;

				uint32_t h = hashTexel(face, x, y);
				if (dir.y > -0.1f && h % 1000u < 3u)
				{
					float brightness = 0.6f + 0.4f * ((h >> 10) & 65535u) / 65535.0f;
					glm::vec3 tint = (h & 0x80000000u) ?
						glm::vec3(0.85f, 0.92f, 1.0f) : glm::vec3(1.0f, 0.93f, 0.82f);
					colour = tint * brightness;
				}
				size_t index = ((size_t)y * size + x) * 3;
				for (int c = 0; c < 3; c++)
					pixels[index + c] = (GLubyte)(std::clamp(colour[c], 0.0f, 1.0f) * 255.0f + 0.5f);
			}
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB8,
			size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
	glBindTexture(GL_TEXTURE_CUBE_MAP, previousTexture);
	return texture;
}
