#include "ControlsOverlay.h"

#include <algorithm>
#include <string>
#include <vector>

#include "PixelFont.h"
#include "shaderClass.h"

namespace
{
struct Row { const char* key; const char* action; };

const Row SCENE_ROWS[] = {
	{ "SPACE, CLICK", "SHAKE GLOBE" },
	{ "L", "HUT LIGHTS" },
	{ "F", "CAMPFIRE" },
	{ "I", "DAY / NIGHT" },
	{ ";", "FPS COUNTER" },
	{ "ESC", "QUIT" },
};
const Row CAMERA_ROWS[] = {
	{ "C", "ORBIT / FREE" },
	{ "P", "PROJECTION" },
	{ "ARROWS", "ORBIT / LOOK" },
	{ "+ -", "ZOOM / SPEED" },
	{ "W A S D", "MOVE (FREE)" },
	{ "Q E", "DOWN / UP" },
	{ "RIGHT DRAG", "LOOK (FREE)" },
};

const int PIXEL = 2;
const int LINE = pixelfont::GLYPH_H * PIXEL + 8;
const int PAD_X = 10, PAD_Y = 9;
const int KEY_GAP = 3 * PIXEL * (pixelfont::GLYPH_W + 1) / 2;
const int GROUP_GAP = 10, HEADER_GAP = 14;
const float PANEL_X = 12.0f, PANEL_Y = 52.0f;
const float MARGIN = 12.0f;

const float BACKDROP[4] = { 0.02f, 0.03f, 0.04f, 0.74f };
const float BRIGHT[4] = { 0.98f, 0.98f, 1.0f, 1.0f };
const float DIM[4] = { 0.72f, 0.76f, 0.84f, 1.0f };

// One static buffer per box: x, y per vertex in window pixels
void Upload(GLuint& vao, GLuint& vbo, const std::vector<float>& data)
{
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
} // namespace

void ControlsOverlay::Init()
{
	shader = new Shader("shaders/ui.vert", "shaders/fps_overlay.frag");

	// Panel: "CONTROLS   H: HIDE", then the scene and camera lists.
	{
		std::vector<float> bright, dim;
		int keyWidth = 0, actionWidth = 0;
		for (const Row& r : SCENE_ROWS) {
			keyWidth = std::max(keyWidth, pixelfont::TextWidth(r.key, PIXEL));
			actionWidth = std::max(actionWidth, pixelfont::TextWidth(r.action, PIXEL));
		}
		for (const Row& r : CAMERA_ROWS) {
			keyWidth = std::max(keyWidth, pixelfont::TextWidth(r.key, PIXEL));
			actionWidth = std::max(actionWidth, pixelfont::TextWidth(r.action, PIXEL));
		}
		const float width = (float)(PAD_X + keyWidth + KEY_GAP + actionWidth + PAD_X);

		float y = (float)PAD_Y;
		pixelfont::AddText(bright, "CONTROLS", (float)PAD_X, y, PIXEL);
		const std::string hide = "H: HIDE";
		pixelfont::AddText(dim, hide, width - PAD_X - pixelfont::TextWidth(hide, PIXEL), y, PIXEL);
		y += pixelfont::GLYPH_H * PIXEL + HEADER_GAP;

		auto AddRows = [&](const Row* rows, size_t count) {
			for (size_t i = 0; i < count; i++)
			{
				pixelfont::AddText(bright, rows[i].key, (float)PAD_X, y, PIXEL);
				pixelfont::AddText(dim, rows[i].action, (float)(PAD_X + keyWidth + KEY_GAP), y, PIXEL);
				y += LINE;
			}
		};
		AddRows(SCENE_ROWS, sizeof(SCENE_ROWS) / sizeof(Row));
		y += GROUP_GAP;
		AddRows(CAMERA_ROWS, sizeof(CAMERA_ROWS) / sizeof(Row));
		const float height = y - LINE + pixelfont::GLYPH_H * PIXEL + PAD_Y;

		std::vector<float> all;
		pixelfont::AddRect(all, 0.0f, 0.0f, width, height);
		all.insert(all.end(), bright.begin(), bright.end());
		all.insert(all.end(), dim.begin(), dim.end());
		panel.backdrop = 6;
		panel.bright = (GLsizei)(bright.size() / 2);
		panel.dim = (GLsizei)(dim.size() / 2);
		panel.width = width;
		panel.height = height;
		Upload(panel.vao, panel.vbo, all);
	}

	// "H: CONTROLS".
	{
		std::vector<float> bright, dim;
		pixelfont::AddText(bright, "H:", (float)PAD_X, (float)PAD_Y, PIXEL);
		pixelfont::AddText(dim, "CONTROLS", (float)(PAD_X + pixelfont::TextWidth("H: ", PIXEL) + PIXEL),
			(float)PAD_Y, PIXEL);
		const float width = (float)(PAD_X + pixelfont::TextWidth("H: CONTROLS", PIXEL) + PAD_X);
		const float height = (float)(PAD_Y + pixelfont::GLYPH_H * PIXEL + PAD_Y);

		std::vector<float> all;
		pixelfont::AddRect(all, 0.0f, 0.0f, width, height);
		all.insert(all.end(), bright.begin(), bright.end());
		all.insert(all.end(), dim.begin(), dim.end());
		hint.backdrop = 6;
		hint.bright = (GLsizei)(bright.size() / 2);
		hint.dim = (GLsizei)(dim.size() / 2);
		hint.width = width;
		hint.height = height;
		Upload(hint.vao, hint.vbo, all);
	}
}

void ControlsOverlay::Draw(int width, int height, float alpha) const
{
	if (!shader || alpha <= 0.0f || width <= 0 || height <= 0) return;

	GLint oldProgram = 0, oldVao = 0;
	GLint oldSrcRgb = 0, oldDstRgb = 0, oldSrcAlpha = 0, oldDstAlpha = 0;
	GLboolean oldDepthWrite = GL_TRUE;
	const GLboolean hadDepth = glIsEnabled(GL_DEPTH_TEST);
	const GLboolean hadBlend = glIsEnabled(GL_BLEND);
	const GLboolean hadCull = glIsEnabled(GL_CULL_FACE);
	glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVao);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthWrite);
	glGetIntegerv(GL_BLEND_SRC_RGB, &oldSrcRgb);
	glGetIntegerv(GL_BLEND_DST_RGB, &oldDstRgb);
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldSrcAlpha);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &oldDstAlpha);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (visible)
		DrawBlock(panel, PANEL_X, PANEL_Y, width, height, alpha);
	else
		DrawBlock(hint, MARGIN, (float)height - MARGIN - hint.height, width, height, alpha);

	glBindVertexArray((GLuint)oldVao);
	glUseProgram((GLuint)oldProgram);
	glBlendFuncSeparate(oldSrcRgb, oldDstRgb, oldSrcAlpha, oldDstAlpha);
	if (!hadBlend) glDisable(GL_BLEND);
	if (hadCull) glEnable(GL_CULL_FACE);
	if (hadDepth) glEnable(GL_DEPTH_TEST);
	glDepthMask(oldDepthWrite);
}

void ControlsOverlay::DrawBlock(const Block& block, float x, float y, int width, int height, float alpha) const
{
	const GLuint id = shader->ID;
	shader->Activate();
	glUniform2f(glGetUniformLocation(id, "screen"), (float)width, (float)height);
	glUniform2f(glGetUniformLocation(id, "origin"), x, y);
	glUniform2f(glGetUniformLocation(id, "scale"), 1.0f, 1.0f);
	glUniform2f(glGetUniformLocation(id, "offset"), 0.0f, 0.0f);
	const GLint color = glGetUniformLocation(id, "hudColor");
	glBindVertexArray(block.vao);

	glUniform4f(color, BACKDROP[0], BACKDROP[1], BACKDROP[2], BACKDROP[3] * alpha);
	glDrawArrays(GL_TRIANGLES, 0, block.backdrop);
	glUniform4f(color, BRIGHT[0], BRIGHT[1], BRIGHT[2], BRIGHT[3] * alpha);
	glDrawArrays(GL_TRIANGLES, block.backdrop, block.bright);
	glUniform4f(color, DIM[0], DIM[1], DIM[2], DIM[3] * alpha);
	glDrawArrays(GL_TRIANGLES, block.backdrop + block.bright, block.dim);
}
