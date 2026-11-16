#ifndef CAMERA_CLASS_H
#define CAMERA_CLASS_H

#include <array>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shaderClass.h"

// A point in cylindrical coordinates around a center (cylinder axis = Y).
struct CylinderPoint
{
    float angle;     // Degrees
    float radius;
    float height;

    glm::vec3 At(const glm::vec3& center) const;
};

// C switches between ORBIT and FREE camera modes.
// Held keys are integrated with dt, so movement is independent of key repeat and FPS.
class Camera
{
public:
    enum Mode { ORBIT, FREE };

    glm::vec3 Position;
    glm::vec3 Orientation;
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

    int width;
    int height;
    Mode mode = ORBIT;

    // ORBIT state
    glm::vec3 target;
    CylinderPoint orbit;
    float sceneRadius;
    float minRadiusK = 1.6f, maxRadiusK = 6.0f;
    float minHeightK = -0.3f, maxHeightK = 2.5f;

    // FREE state
    float yaw = -90.0f;
    float pitch = 0.0f;
    float moveSpeed = 1.0f;                 // Adjustable with +/-
    float mouseSensitivity = 0.2f;

    bool perspective = true;
    float fovDeg = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    Camera(int width, int height, glm::vec3 target, float sceneRadius);

    glm::mat4 ViewMatrix() const;
    glm::mat4 ProjectionMatrix() const;
    void Matrix(float FOVdeg, float nearPlane, float farPlane, Shader& shader, const char* uniform);

    // Key-down/key-up callbacks. Key-up callbacks MUST be registered in main.cpp.
    bool Inputs(unsigned char key);
    void KeyUp(unsigned char key);
    bool SpecialInputs(int key);
    void SpecialUp(int key);
    void MouseLook(int dx, int dy);

    // Call once per frame from idle(), before building the rendering frame.
    void Update(float dt);
    void Resize(int newWidth, int newHeight);
    void ToggleMode();
    void ClampToSphere(const glm::vec3& center, float softRadius, float easeSpeed, float dt);

private:
    std::array<bool, 256> heldKeys{};
    std::array<bool, 4> heldArrows{};  // Left, Right, Up, Down

    glm::vec3 velocity = glm::vec3(0.0f);
    float orbitAngleVelocity = 0.0f;
    float orbitHeightVelocity = 0.0f;
    float orbitZoomVelocity = 0.0f;
    float targetYaw = -90.0f;
    float targetPitch = 0.0f;

    void UpdatePosition();              // Used only for the initial orbit pose.
    void UpdateOrientation();
    void ResetMotion();
};

#endif
