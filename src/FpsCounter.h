#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include <glad/glad.h>
#include <string>

class Shader;

// OpenGL 3.3 HUD. Both title and overlay are toggled with ';'.
// Does not use deprecated GLUT bitmap characters or fixed-function drawing.
class FpsCounter
{
public:
    void Init();
    bool Frame(int elapsedMilliseconds);  // Returns true when a new FPS sample is ready.
    void Toggle() { visible = !visible; }
    bool Visible() const { return visible; }
    std::string Title() const;
    void Draw(int width, int height) const;

private:
    Shader* shader = nullptr;
    GLuint vao = 0;
    GLuint vbo = 0;
    int frames = 0;
    int lastSample = -1;
    float fps = 0.0f;
    bool visible = false; // Press ';' once to show both counters.
};

#endif
