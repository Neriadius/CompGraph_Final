#include "Nameplate.h"

#include <cmath>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "SceneLights.h"
#include "shaderClass.h"
#include "Texture.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"

namespace
{
    constexpr float OUTER_RADIUS = 1.220f; // Wood side band is at radius 1.20.
    constexpr float INNER_RADIUS = 1.204f; // Slightly proud of the wood; avoids z-fighting.
    constexpr float HALF_ARC = 0.41f;      // Radians, centred on +Z (front camera).
    constexpr float BOTTOM_Y = -1.455f;
    constexpr float TOP_Y = -1.315f;
    constexpr int SLICES = 40;

    struct Mesh
    {
        std::vector<GLfloat> vertices; // position(3), normal(3), UV(2)
        std::vector<GLuint> indices;

        GLuint Add(float angle, float y, float radius, const glm::vec3& n, float u, float v)
        {
            GLuint id = static_cast<GLuint>(vertices.size() / 8);
            vertices.insert(vertices.end(), {
                radius * std::sin(angle), y, radius * std::cos(angle),
                n.x, n.y, n.z, u, v
            });
            return id;
        }

        void Quad(GLuint a, GLuint b, GLuint c, GLuint d)
        {
            indices.insert(indices.end(), { a, b, c, a, c, d });
        }
    };

    void BuildNameplate(Mesh& mesh)
    {
        // Front, curved to match the cylindrical lower band of the base.
        for (int i = 0; i < SLICES; ++i)
        {
            float a = -HALF_ARC + 2.0f * HALF_ARC * i / SLICES;
            float b = -HALF_ARC + 2.0f * HALF_ARC * (i + 1) / SLICES;
            float u0 = static_cast<float>(i) / SLICES;
            float u1 = static_cast<float>(i + 1) / SLICES;
            glm::vec3 na(std::sin(a), 0.0f, std::cos(a));
            glm::vec3 nb(std::sin(b), 0.0f, std::cos(b));
            GLuint bl = mesh.Add(a, BOTTOM_Y, OUTER_RADIUS, na, u0, 0.0f);
            GLuint br = mesh.Add(b, BOTTOM_Y, OUTER_RADIUS, nb, u1, 0.0f);
            GLuint tr = mesh.Add(b, TOP_Y, OUTER_RADIUS, nb, u1, 1.0f);
            GLuint tl = mesh.Add(a, TOP_Y, OUTER_RADIUS, na, u0, 1.0f);
            mesh.Quad(bl, br, tr, tl); // CCW faces toward the camera at +Z.

            // Narrow metal edges, not a painted rectangle floating over the wood.
            // Top edge faces +Y; bottom edge faces -Y.
            GLuint ti0 = mesh.Add(a, TOP_Y, INNER_RADIUS, { 0, 1, 0 }, u0, 0.98f);
            GLuint to0 = mesh.Add(a, TOP_Y, OUTER_RADIUS, { 0, 1, 0 }, u0, 0.98f);
            GLuint to1 = mesh.Add(b, TOP_Y, OUTER_RADIUS, { 0, 1, 0 }, u1, 0.98f);
            GLuint ti1 = mesh.Add(b, TOP_Y, INNER_RADIUS, { 0, 1, 0 }, u1, 0.98f);
            mesh.Quad(ti0, to0, to1, ti1);
            GLuint bi0 = mesh.Add(a, BOTTOM_Y, INNER_RADIUS, { 0, -1, 0 }, u0, 0.02f);
            GLuint bi1 = mesh.Add(b, BOTTOM_Y, INNER_RADIUS, { 0, -1, 0 }, u1, 0.02f);
            GLuint bo1 = mesh.Add(b, BOTTOM_Y, OUTER_RADIUS, { 0, -1, 0 }, u1, 0.02f);
            GLuint bo0 = mesh.Add(a, BOTTOM_Y, OUTER_RADIUS, { 0, -1, 0 }, u0, 0.02f);
            mesh.Quad(bi0, bi1, bo1, bo0);
        }

        // Close both sides with a visible metal thickness.
        for (int side = 0; side < 2; ++side)
        {
            float a = side ? HALF_ARC : -HALF_ARC;
            glm::vec3 n(side ? std::cos(a) : -std::cos(a), 0.0f,
                        side ? -std::sin(a) : std::sin(a));
            float u = side ? 0.99f : 0.01f;
            GLuint bInner = mesh.Add(a, BOTTOM_Y, INNER_RADIUS, n, u, 0.02f);
            GLuint tInner = mesh.Add(a, TOP_Y, INNER_RADIUS, n, u, 0.98f);
            GLuint tOuter = mesh.Add(a, TOP_Y, OUTER_RADIUS, n, u, 0.98f);
            GLuint bOuter = mesh.Add(a, BOTTOM_Y, OUTER_RADIUS, n, u, 0.02f);
            if (side) mesh.Quad(bInner, tInner, tOuter, bOuter);
            else      mesh.Quad(bOuter, tOuter, tInner, bInner);
        }
    }
}

void Nameplate::Init()
{
    shader = new Shader("shaders/textured.vert", "shaders/nameplate.frag");
    texture = new Texture();
    texture->Load("textures/vinter_varme_plate.png", GL_CLAMP_TO_EDGE, false, true);
    shader->Activate();
    Texture::Uniform(*shader, "tex0", 0);

    Mesh mesh;
    BuildNameplate(mesh);
    indexCount = static_cast<GLsizei>(mesh.indices.size());
    vao = new VAO();
    vao->Bind();
    VBO vbo(mesh.vertices.data(), static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(GLfloat)));
    EBO ebo(mesh.indices.data(), static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(GLuint)));
    vao->LinkAttrib(vbo, 0, 3, GL_FLOAT, 8 * sizeof(GLfloat), (void*)0);
    vao->LinkAttrib(vbo, 1, 3, GL_FLOAT, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    vao->LinkAttrib(vbo, 2, 2, GL_FLOAT, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
}

void Nameplate::Draw(const FrameContext& frame, Camera& camera, const SceneLights& lights)
{
    glm::mat4 model = glm::scale(frame.globeModel, glm::vec3(frame.globeRadius));
    shader->Activate();
    camera.Matrix(45.0f, frame.globeRadius * 0.01f, frame.globeRadius * 50.0f, *shader, "camMatrix");
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shader->ID, "camPos"), 1, glm::value_ptr(frame.camPos));
    UploadSceneLights(shader->ID, lights);
    texture->Bind(0);
    vao->Bind();
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
}
