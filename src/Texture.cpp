#include "Texture.h"

#include <iostream>
#include <string>
#include <vector>

#include "Image.h"

// ---------------------------------------------------------------
// OpenGL side
// ---------------------------------------------------------------
void Texture::Upload(const unsigned char* rgba, int w, int h, GLenum wrap, bool nearest, bool mipmaps)
{
	width = w;
	height = h;

	GLint previousAlignment;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (ID == 0) glGenTextures(1, &ID);
	glBindTexture(GL_TEXTURE_2D, ID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest ? GL_NEAREST : GL_LINEAR);
	if (mipmaps)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest ? GL_NEAREST : GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	if (mipmaps) glGenerateMipmap(GL_TEXTURE_2D);

	glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
	glBindTexture(GL_TEXTURE_2D, 0);
}

bool Texture::Load(const char* path, GLenum wrap, bool nearest, bool mipmaps)
{
	Image img;

	if (loadImage(path, img))
	{
		// Image: RGB, first row = TOP.  OpenGL wants RGBA, first row = BOTTOM.
		std::vector<unsigned char> pixels((size_t)img.width * img.height * 4, 255);
		for (int y = 0; y < img.height; y++)
		{
			const unsigned char* src = &img.rgb[(size_t)(img.height - 1 - y) * img.width * 3];
			unsigned char* dst = &pixels[(size_t)y * img.width * 4];
			for (int x = 0; x < img.width; x++)
			{
				dst[x * 4 + 0] = src[x * 3 + 0];
				dst[x * 4 + 1] = src[x * 3 + 1];
				dst[x * 4 + 2] = src[x * 3 + 2];
			}
		}
		Upload(pixels.data(), img.width, img.height, wrap, nearest, mipmaps);
		return true;
	}

	std::cerr << "Texture: failed to load \"" << path << "\", using a checkerboard instead" << std::endl;

	// Fallback so a missing file is obvious on screen instead of crashing
	const int n = 8;
	std::vector<unsigned char> check((size_t)n * n * 4);
	for (int y = 0; y < n; y++)
		for (int x = 0; x < n; x++)
		{
			bool on = ((x / 2) + (y / 2)) % 2 == 0;
			unsigned char* p = &check[((size_t)y * n + x) * 4];
			p[0] = on ? 255 : 0; p[1] = 0; p[2] = on ? 255 : 0; p[3] = 255;
		}
	Upload(check.data(), n, n, wrap, true, mipmaps);
	return false;
}

void Texture::Bind(GLuint unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::Unbind() const
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Delete()
{
	if (ID) glDeleteTextures(1, &ID);
	ID = 0;
}

void Texture::Uniform(Shader& shader, const char* uniform, GLuint unit)
{
	glUniform1i(glGetUniformLocation(shader.ID, uniform), (GLint)unit);
}
