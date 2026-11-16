#ifndef NAMEPLATE_H
#define NAMEPLATE_H

#include <glad/glad.h>
#include "FrameContext.h"

class Camera;
class SceneLights;
class Shader;
class Texture;
class VAO;

// Curved brass nameplate mounted on the front of the wooden base.
// Its world transform follows the shake, as the base does.
class Nameplate
{
public:
    void Init();
    void Draw(const FrameContext& frame, Camera& camera, const SceneLights& lights);

private:
    Shader* shader = nullptr;
    Texture* texture = nullptr;
    VAO* vao = nullptr;
    GLsizei indexCount = 0;
};

#endif
