#ifndef CONTROLS_OVERLAY_H
#define CONTROLS_OVERLAY_H

#include <glad/glad.h>

class Shader;

// In-scene controls list
class ControlsOverlay
{
public:
	void Init();
	void Toggle() { visible = !visible; }
	bool Visible() const { return visible; }

	// alpha fades the overlay in after the start menu (0 = not drawn).
	void Draw(int width, int height, float alpha) const;

private:
	struct Block
	{
		GLuint vao = 0, vbo = 0;
		GLsizei backdrop = 0, bright = 0, dim = 0;  // vertex counts, stored in this order
		float width = 0.0f, height = 0.0f;
	};

	Shader* shader = nullptr;
	Block panel, hint;
	bool visible = true;

	void DrawBlock(const Block& block, float x, float y, int width, int height, float alpha) const;
};

#endif
