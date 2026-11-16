#include "PixelFont.h"

#include <cctype>

namespace pixelfont
{
namespace
{
struct Entry { char c; std::uint8_t rows[GLYPH_H]; };

const Entry GLYPHS[] = {
	{ '0', {14,17,19,21,25,17,14} }, { '1', { 4,12, 4, 4, 4, 4,14} },
	{ '2', {14,17, 1, 2, 4, 8,31} }, { '3', {30, 1, 1,14, 1, 1,30} },
	{ '4', { 2, 6,10,18,31, 2, 2} }, { '5', {31,16,30, 1, 1,17,14} },
	{ '6', { 6, 8,16,30,17,17,14} }, { '7', {31, 1, 2, 4, 8, 8, 8} },
	{ '8', {14,17,17,14,17,17,14} }, { '9', {14,17,17,15, 1, 2,12} },

	{ 'A', {14,17,17,17,31,17,17} }, { 'B', {30,17,17,30,17,17,30} },
	{ 'C', {14,17,16,16,16,17,14} }, { 'D', {28,18,17,17,17,18,28} },
	{ 'E', {31,16,16,30,16,16,31} }, { 'F', {31,16,16,30,16,16,16} },
	{ 'G', {14,17,16,23,17,17,15} }, { 'H', {17,17,17,31,17,17,17} },
	{ 'I', {14, 4, 4, 4, 4, 4,14} }, { 'J', { 7, 2, 2, 2, 2,18,12} },
	{ 'K', {17,18,20,24,20,18,17} }, { 'L', {16,16,16,16,16,16,31} },
	{ 'M', {17,27,21,21,17,17,17} }, { 'N', {17,17,25,21,19,17,17} },
	{ 'O', {14,17,17,17,17,17,14} }, { 'P', {30,17,17,30,16,16,16} },
	{ 'Q', {14,17,17,17,21,18,13} }, { 'R', {30,17,17,30,20,18,17} },
	{ 'S', {15,16,16,14, 1, 1,30} }, { 'T', {31, 4, 4, 4, 4, 4, 4} },
	{ 'U', {17,17,17,17,17,17,14} }, { 'V', {17,17,17,17,17,10, 4} },
	{ 'W', {17,17,17,21,21,21,10} }, { 'X', {17,17,10, 4,10,17,17} },
	{ 'Y', {17,17,17,10, 4, 4, 4} }, { 'Z', {31, 1, 2, 4, 8,16,31} },

	{ ':', { 0, 4, 4, 0, 4, 4, 0} }, { '.', { 0, 0, 0, 0, 0, 6, 6} },
	{ ';', { 0, 4, 4, 0, 4, 4, 8} }, { ',', { 0, 0, 0, 0, 4, 4, 8} },
	{ '/', { 0, 1, 2, 4, 8,16, 0} }, { '+', { 0, 4, 4,31, 4, 4, 0} },
	{ '-', { 0, 0, 0,31, 0, 0, 0} }, { '=', { 0, 0,31, 0,31, 0, 0} },
	{ '(', { 2, 4, 8, 8, 8, 4, 2} }, { ')', { 8, 4, 2, 2, 2, 4, 8} },
	{ '[', {14, 8, 8, 8, 8, 8,14} }, { ']', {14, 2, 2, 2, 2, 2,14} },
	{ '<', { 2, 4, 8,16, 8, 4, 2} }, { '>', { 8, 4, 2, 1, 2, 4, 8} },
	{ '!', { 4, 4, 4, 4, 4, 0, 4} }, { '?', {14,17, 1, 2, 4, 0, 4} },
	{ '\'', { 4, 4, 8, 0, 0, 0, 0} }, { '_', { 0, 0, 0, 0, 0, 0,31} },
};
const std::uint8_t EMPTY[GLYPH_H] = { 0, 0, 0, 0, 0, 0, 0 };
} // namespace

const std::uint8_t* Glyph(char c)
{
	c = (char)std::toupper((unsigned char)c);
	for (const Entry& e : GLYPHS)
		if (e.c == c) return e.rows;
	return EMPTY;
}

int TextWidth(const std::string& text, int pixel)
{
	if (text.empty()) return 0;
	return ((int)text.size() * (GLYPH_W + 1) - 1) * pixel;
}

void AddRect(std::vector<float>& out, float x0, float y0, float x1, float y1)
{
	const float quad[] = { x0, y0, x1, y0, x1, y1, x0, y0, x1, y1, x0, y1 };
	out.insert(out.end(), quad, quad + 12);
}

void AddText(std::vector<float>& out, const std::string& text, float x, float y, int pixel)
{
	for (size_t i = 0; i < text.size(); i++)
	{
		const std::uint8_t* rows = Glyph(text[i]);
		const float left = x + (float)(i * (GLYPH_W + 1) * pixel);
		for (int row = 0; row < GLYPH_H; row++)
			for (int col = 0; col < GLYPH_W; col++)
				if (rows[row] & (1 << (GLYPH_W - 1 - col)))
				{
					const float px = left + (float)(col * pixel);
					const float py = y + (float)(row * pixel);
					AddRect(out, px, py, px + pixel, py + pixel);
				}
	}
}
} // namespace pixelfont
