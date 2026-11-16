#ifndef UI_FONT_H
#define UI_FONT_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <glad/glad.h>

#include "Texture.h"

// Signed distance field (SDF) font for the menu and the controls overlay
// textures/ui_font.png + textures/ui_font.txt are baked by tools/make_font_atlas.py
enum class UiFace { Title = 0, Body = 1, Bold = 2 };

struct TextMesh
{
	GLuint vao = 0;
	GLuint vbo = 0;
	GLsizei vertexCount = 0;
	float width = 0.0f;          // in em (multiplied by the size in pixels)
	UiFace face = UiFace::Body;
};

class UiFont
{
public:
	bool Init(const char* atlasPath, const char* metricsPath);

	// UTF-8 text, pen starts at x = 0 on the baseline. tracking = extra space per letter, in em.
	TextMesh Build(UiFace face, const std::string& text, float tracking = 0.0f) const;

	float EmPx(UiFace face) const { return faces[(int)face].emPx; }
	float SpreadPx(UiFace face) const { return faces[(int)face].spreadPx; }
	float CapHeight(UiFace face) const { return faces[(int)face].capHeight; }  // in em
	void Bind(GLuint unit) const { atlas.Bind(unit); }

private:
	struct Glyph { float x0, y0, x1, y1, u0, v0, u1, v1, advance; };
	struct Face
	{
		float emPx = 1.0f, spreadPx = 1.0f, capHeight = 0.7f;
		std::unordered_map<std::uint32_t, Glyph> glyphs;
		std::unordered_map<std::uint64_t, float> kerning;
	};

	Face faces[3];
	Texture atlas;
};

#endif
