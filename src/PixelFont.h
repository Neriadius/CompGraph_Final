#ifndef PIXEL_FONT_H
#define PIXEL_FONT_H

#include <cstdint>
#include <string>
#include <vector>

// 5 x 7 pixel font shared by the FPS counter and the controls overlay, so all on-screen HUD text looks the same
namespace pixelfont
{
constexpr int GLYPH_W = 5;
constexpr int GLYPH_H = 7;

// 7 rows, top row first; bit 4 is the left column
const std::uint8_t* Glyph(char c);

// Width in pixels of text drawn with the given pixel size
int TextWidth(const std::string& text, int pixel);

// Appends two triangles (x, y in window pixels, y down) for every lit font pixel
void AddText(std::vector<float>& out, const std::string& text, float x, float y, int pixel);

// Appends one rectangle as two triangles, same format
void AddRect(std::vector<float>& out, float x0, float y0, float x1, float y1);
}

#endif
