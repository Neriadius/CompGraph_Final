#include "UiRenderer.h"

#include <glm/gtc/type_ptr.hpp>

#include "shaderClass.h"

namespace
{
bool Linked(const Shader* shader)
{
	GLint ok = GL_FALSE;
	glGetProgramiv(shader->ID, GL_LINK_STATUS, &ok);
	return ok == GL_TRUE;
}
} // namespace

bool UiRenderer::Init()
{
	rectShader = new Shader("shaders/ui.vert", "shaders/ui_rect.frag");
	textShader = new Shader("shaders/ui.vert", "shaders/ui_text.frag");
	textShader->Activate();
	glUniform1i(glGetUniformLocation(textShader->ID, "atlas"), 0);

	// Unit quad for rectangles (two triangles, the uv attribute is unused).
	const float quad[] = {
		0, 0, 0, 0,  1, 0, 0, 0,  1, 1, 0, 0,
		0, 0, 0, 0,  1, 1, 0, 0,  0, 1, 0, 0,
	};
	glGenVertexArrays(1, &quadVao);
	glGenBuffers(1, &quadVbo);
	glBindVertexArray(quadVao);
	glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	const bool fontOk = font.Init("textures/ui_font.png", "textures/ui_font.txt");
	return fontOk && Linked(rectShader) && Linked(textShader);
}

void UiRenderer::Begin(int width, int height)
{
	screen = glm::vec2((float)width, (float)height);

	glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVao);
	glGetIntegerv(GL_BLEND_SRC_RGB, &oldSrcRgb);
	glGetIntegerv(GL_BLEND_DST_RGB, &oldDstRgb);
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldSrcAlpha);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &oldDstAlpha);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthWrite);
	hadDepth = glIsEnabled(GL_DEPTH_TEST);
	hadBlend = glIsEnabled(GL_BLEND);
	hadCull = glIsEnabled(GL_CULL_FACE);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void UiRenderer::End()
{
	glBindVertexArray((GLuint)oldVao);
	glUseProgram((GLuint)oldProgram);
	glBlendFuncSeparate(oldSrcRgb, oldDstRgb, oldSrcAlpha, oldDstAlpha);
	if (!hadBlend) glDisable(GL_BLEND);
	if (hadCull) glEnable(GL_CULL_FACE);
	if (hadDepth) glEnable(GL_DEPTH_TEST);
	glDepthMask(oldDepthWrite);
}

void UiRenderer::Rect(float x, float y, float w, float h, float radius, const glm::vec4& fill,
	const glm::vec4& border, float borderPx, float softPx)
{
	if (fill.a <= 0.0f && (border.a <= 0.0f || borderPx <= 0.0f)) return;
	const float pad = softPx + 1.0f;       // room for the soft / anti-aliased edge
	const GLuint id = rectShader->ID;
	rectShader->Activate();
	glUniform2f(glGetUniformLocation(id, "screen"), screen.x, screen.y);
	glUniform2f(glGetUniformLocation(id, "origin"), x, y);
	glUniform2f(glGetUniformLocation(id, "scale"), w + 2.0f * pad, h + 2.0f * pad);
	glUniform2f(glGetUniformLocation(id, "offset"), -pad, -pad);
	glUniform2f(glGetUniformLocation(id, "size"), w, h);
	glUniform1f(glGetUniformLocation(id, "radius"), glm::min(radius, 0.5f * glm::min(w, h)));
	glUniform4fv(glGetUniformLocation(id, "fill"), 1, glm::value_ptr(fill));
	glUniform4fv(glGetUniformLocation(id, "border"), 1, glm::value_ptr(border));
	glUniform1f(glGetUniformLocation(id, "borderPx"), borderPx);
	glUniform1f(glGetUniformLocation(id, "softPx"), glm::max(softPx, 1.0f));
	glBindVertexArray(quadVao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void UiRenderer::TextPass(const TextMesh& text, float x, float y, float sizePx, const glm::vec4& color,
	float edgePx, float softPx)
{
	const GLuint id = textShader->ID;
	const float pxPerUnit = 2.0f * font.SpreadPx(text.face) * sizePx / font.EmPx(text.face);
	textShader->Activate();
	glUniform2f(glGetUniformLocation(id, "screen"), screen.x, screen.y);
	glUniform2f(glGetUniformLocation(id, "origin"), x, y);
	glUniform2f(glGetUniformLocation(id, "scale"), sizePx, sizePx);
	glUniform2f(glGetUniformLocation(id, "offset"), 0.0f, 0.0f);
	glUniform4fv(glGetUniformLocation(id, "color"), 1, glm::value_ptr(color));
	glUniform1f(glGetUniformLocation(id, "pxPerUnit"), pxPerUnit);
	glUniform1f(glGetUniformLocation(id, "edgePx"), edgePx);
	glUniform1f(glGetUniformLocation(id, "softPx"), softPx);
	font.Bind(0);
	glBindVertexArray(text.vao);
	glDrawArrays(GL_TRIANGLES, 0, text.vertexCount);
}

void UiRenderer::Text(const TextMesh& text, float x, float y, float sizePx, const glm::vec4& color,
	float glowPx, const glm::vec4& glowColor)
{
	if (text.vertexCount == 0 || color.a <= 0.0f) return;
	// The halo can reach as far as the distance field goes (spread, scaled to the screen).
	const float maxGlow = 0.9f * font.SpreadPx(text.face) * sizePx / font.EmPx(text.face);
	glowPx = glm::min(glowPx, maxGlow);
	if (glowPx > 0.5f && glowColor.a > 0.0f)
		TextPass(text, x, y, sizePx, glowColor, 0.5f * glowPx, glowPx);
	TextPass(text, x, y, sizePx, color, 0.0f, 1.0f);
}

void UiRenderer::TextCentered(const TextMesh& text, float cx, float cy, float sizePx, const glm::vec4& color,
	float glowPx, const glm::vec4& glowColor)
{
	const float x = cx - 0.5f * text.width * sizePx;
	const float y = cy + 0.5f * font.CapHeight(text.face) * sizePx;
	Text(text, x, y, sizePx, color, glowPx, glowColor);
}
