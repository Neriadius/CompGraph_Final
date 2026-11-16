#include "FpsCounter.h"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>

#include "PixelFont.h"
#include "shaderClass.h"

namespace
{
constexpr int GLYPH_WIDTH = pixelfont::GLYPH_W;   // shared 5x7 font (src/PixelFont)
constexpr int GLYPH_HEIGHT = pixelfont::GLYPH_H;
constexpr int PIXEL_SCALE = 3;
constexpr int TEXT_X = 22;
constexpr int TEXT_Y = 21;

// Append two triangles for a rectangle in window pixel coordinates.
void AddRect(std::vector<float>& vertices, float x0, float y0, float x1, float y1, int w, int h)
{
    const float left = 2.0f * x0 / w - 1.0f;
    const float right = 2.0f * x1 / w - 1.0f;
    const float top = 1.0f - 2.0f * y0 / h;
    const float bottom = 1.0f - 2.0f * y1 / h;
    const float data[] = {
        left, top, right, top, right, bottom,
        left, top, right, bottom, left, bottom
    };
    vertices.insert(vertices.end(), data, data + 12);
}
} // namespace

void FpsCounter::Init()
{
    shader = new Shader("shaders/fps_overlay.vert", "shaders/fps_overlay.frag");
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool FpsCounter::Frame(int elapsedMilliseconds)
{
    if (lastSample < 0) lastSample = elapsedMilliseconds;
    ++frames;
    int delta = elapsedMilliseconds - lastSample;
    if (delta < 500) return false;

    fps = frames * 1000.0f / delta;
    frames = 0;
    lastSample = elapsedMilliseconds;
    return true;
}

std::string FpsCounter::Title() const
{
    if (!visible) return "Snow Globe";
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "Snow Globe | FPS: %.1f", fps);
    return buffer;
}

void FpsCounter::Draw(int width, int height) const
{
    if (!visible || !shader || width <= 0 || height <= 0) return;

    char text[48];
    std::snprintf(text, sizeof(text), "FPS: %.1f", fps);
    const int n = static_cast<int>(std::strlen(text));

    std::vector<float> vertices;
    vertices.reserve(12 + n * GLYPH_WIDTH * GLYPH_HEIGHT * 12);
    const int boxWidth = n * (GLYPH_WIDTH + 1) * PIXEL_SCALE + 18;
    AddRect(vertices, 12.0f, 12.0f, 12.0f + boxWidth, 44.0f, width, height);

    for (int i = 0; i < n; ++i)
    {
        const std::uint8_t* glyph = pixelfont::Glyph(text[i]);
        for (int y = 0; y < GLYPH_HEIGHT; ++y)
            for (int x = 0; x < GLYPH_WIDTH; ++x)
                if ((glyph[y] & (1 << (GLYPH_WIDTH - 1 - x))) != 0)
                {
                    float left = static_cast<float>(TEXT_X + (i * (GLYPH_WIDTH + 1) + x) * PIXEL_SCALE);
                    float top = static_cast<float>(TEXT_Y + y * PIXEL_SCALE);
                    AddRect(vertices, left, top, left + PIXEL_SCALE, top + PIXEL_SCALE, width, height);
                }
    }

    GLint oldProgram = 0, oldVao = 0, oldBuffer = 0;
    GLint oldSrcRgb = 0, oldDstRgb = 0, oldSrcAlpha = 0, oldDstAlpha = 0;
    GLboolean oldDepthWrite = GL_TRUE;
    const GLboolean hadDepth = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean hadBlend = glIsEnabled(GL_BLEND);
    const GLboolean hadCull = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldBuffer);
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

    shader->Activate();
    GLint colorLoc = glGetUniformLocation(shader->ID, "hudColor");
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STREAM_DRAW);
    glUniform4f(colorLoc, 0.02f, 0.03f, 0.04f, 0.74f);
    glDrawArrays(GL_TRIANGLES, 0, 6); // translucent backdrop
    glUniform4f(colorLoc, 0.98f, 0.98f, 1.0f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 6, static_cast<GLsizei>(vertices.size() / 2 - 6));

    glBindVertexArray(static_cast<GLuint>(oldVao));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(oldBuffer));
    glUseProgram(static_cast<GLuint>(oldProgram));
    glBlendFuncSeparate(oldSrcRgb, oldDstRgb, oldSrcAlpha, oldDstAlpha);
    if (!hadBlend) glDisable(GL_BLEND);
    if (hadCull) glEnable(GL_CULL_FACE);
    if (hadDepth) glEnable(GL_DEPTH_TEST);
    glDepthMask(oldDepthWrite);
}
