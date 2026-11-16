#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "UiFont.h"

class Shader;

// 2D drawing in window pixels (origin top-left) for the menu and the controls overlay
class UiRenderer
{
public:
	UiFont font;

	bool Init();
	void Begin(int width, int height);
	void End();

	// borderPx = outline width; softPx > 1 blurs the edge (used for soft shadows / glows).
	void Rect(float x, float y, float w, float h, float radius, const glm::vec4& fill,
		const glm::vec4& border = glm::vec4(0.0f), float borderPx = 0.0f, float softPx = 1.0f);

	// (x, y) = start of the baseline. glowPx > 0 first draws a soft halo in glowColor.
	void Text(const TextMesh& text, float x, float y, float sizePx, const glm::vec4& color,
		float glowPx = 0.0f, const glm::vec4& glowColor = glm::vec4(0.0f));

	// Same, centred horizontally on cx, with the capital letters centred vertically on cy.
	void TextCentered(const TextMesh& text, float cx, float cy, float sizePx, const glm::vec4& color,
		float glowPx = 0.0f, const glm::vec4& glowColor = glm::vec4(0.0f));

private:
	Shader* rectShader = nullptr;
	Shader* textShader = nullptr;
	GLuint quadVao = 0, quadVbo = 0;
	glm::vec2 screen = glm::vec2(1.0f);

	// Saved state
	GLint oldProgram = 0, oldVao = 0;
	GLint oldSrcRgb = 0, oldDstRgb = 0, oldSrcAlpha = 0, oldDstAlpha = 0;
	GLboolean oldDepthWrite = GL_TRUE, hadDepth = GL_FALSE, hadBlend = GL_FALSE, hadCull = GL_FALSE;

	void TextPass(const TextMesh& text, float x, float y, float sizePx, const glm::vec4& color,
		float edgePx, float softPx);
};

#endif
