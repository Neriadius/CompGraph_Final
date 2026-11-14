#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>

#include "shaderClass.h"

// 2D texture loader (Person 4). Shared helper: anyone can use it.
//
//   Texture wood;
//   wood.Load("textures/wood.png", GL_REPEAT, true);   // true = crisp pixel look
//   ...
//   shader.Activate();
//   wood.Uniform(shader, "tex0", 0);                    // once, after Load
//   wood.Bind(0);                                       // before drawing
//
// Image files are decoded by src/Image (loadImage): PNG, or binary PPM (P6). No external library.
// If the file cannot be loaded, Load() prints the reason, uses a visible
// magenta/black checkerboard instead and returns false, so the program keeps running.
class Texture
{
public:
	GLuint ID = 0;
	int width = 0;
	int height = 0;

	// wrap    : GL_REPEAT, GL_CLAMP_TO_EDGE, ...
	// nearest : true = GL_NEAREST when magnified (pixel art), false = smooth
	// mipmaps : generate mipmaps (less shimmering when far away)
	bool Load(const char* path, GLenum wrap = GL_REPEAT, bool nearest = false, bool mipmaps = true);

	void Bind(GLuint unit = 0) const;
	void Unbind() const;
	void Delete();

	// Tells the shader which texture unit a sampler2D reads from (shader must be active)
	static void Uniform(Shader& shader, const char* uniform, GLuint unit);

private:
	void Upload(const unsigned char* rgba, int w, int h, GLenum wrap, bool nearest, bool mipmaps);
};

#endif
