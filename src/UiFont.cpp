#include "UiFont.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace
{
// Decodes UTF-8 into code points (enough for the ASCII + dashes and dots in the atlas).
std::vector<std::uint32_t> Decode(const std::string& text)
{
	std::vector<std::uint32_t> codes;
	for (size_t i = 0; i < text.size();)
	{
		unsigned char c = (unsigned char)text[i];
		std::uint32_t code = c;
		int extra = 0;
		if (c >= 0xF0) { code = c & 0x07; extra = 3; }
		else if (c >= 0xE0) { code = c & 0x0F; extra = 2; }
		else if (c >= 0xC0) { code = c & 0x1F; extra = 1; }
		i++;
		for (int k = 0; k < extra && i < text.size(); k++, i++)
			code = (code << 6) | ((unsigned char)text[i] & 0x3F);
		codes.push_back(code);
	}
	return codes;
}

std::uint64_t PairKey(std::uint32_t left, std::uint32_t right)
{
	return ((std::uint64_t)left << 32) | right;
}
} // namespace

bool UiFont::Init(const char* atlasPath, const char* metricsPath)
{
	bool ok = atlas.Load(atlasPath, GL_CLAMP_TO_EDGE, false, false);

	std::ifstream file(metricsPath);
	if (!file)
	{
		std::cerr << "UiFont: cannot open " << metricsPath << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#') continue;
		std::istringstream in(line);
		std::string kind;
		int face = 0;
		in >> kind >> face;
		if (face < 0 || face > 2) continue;

		if (kind == "face")
		{
			float ascent, descent;
			in >> faces[face].emPx >> faces[face].spreadPx >> ascent >> descent;
		}
		else if (kind == "glyph")
		{
			std::uint32_t code;
			Glyph g;
			in >> code >> g.x0 >> g.y0 >> g.x1 >> g.y1 >> g.u0 >> g.v0 >> g.u1 >> g.v1 >> g.advance;
			faces[face].glyphs[code] = g;
		}
		else if (kind == "kern")
		{
			std::uint32_t left, right;
			float amount;
			in >> left >> right >> amount;
			faces[face].kerning[PairKey(left, right)] = amount;
		}
	}

	// Height of a capital H (top of the ink, minus the padding around it) for vertical centring.
	for (Face& f : faces)
	{
		auto it = f.glyphs.find('H');
		if (it != f.glyphs.end())
			f.capHeight = -it->second.y0 - (f.spreadPx + 1.0f) / f.emPx;
	}
	return ok && !faces[0].glyphs.empty();
}

TextMesh UiFont::Build(UiFace faceId, const std::string& text, float tracking) const
{
	const Face& face = faces[(int)faceId];
	std::vector<float> vertices;     // x, y (em), u, v per vertex; 6 vertices per glyph
	float pen = 0.0f;
	std::uint32_t previous = 0;

	for (std::uint32_t code : Decode(text))
	{
		auto it = face.glyphs.find(code);
		if (it == face.glyphs.end()) it = face.glyphs.find('?');
		if (it == face.glyphs.end()) continue;
		const Glyph& g = it->second;

		auto kern = face.kerning.find(PairKey(previous, code));
		if (kern != face.kerning.end()) pen += kern->second;

		if (g.x1 > g.x0)
		{
			const float x0 = pen + g.x0, x1 = pen + g.x1;
			const float quad[6][4] = {
				{ x0, g.y0, g.u0, g.v0 }, { x1, g.y0, g.u1, g.v0 }, { x1, g.y1, g.u1, g.v1 },
				{ x0, g.y0, g.u0, g.v0 }, { x1, g.y1, g.u1, g.v1 }, { x0, g.y1, g.u0, g.v1 },
			};
			for (const auto& v : quad) vertices.insert(vertices.end(), v, v + 4);
		}
		pen += g.advance + tracking;
		previous = code;
	}

	TextMesh mesh;
	mesh.face = faceId;
	mesh.width = pen - (text.empty() ? 0.0f : tracking);
	mesh.vertexCount = (GLsizei)(vertices.size() / 4);

	glGenVertexArrays(1, &mesh.vao);
	glGenBuffers(1, &mesh.vbo);
	glBindVertexArray(mesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertices.size() * sizeof(float)),
		vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return mesh;
}
