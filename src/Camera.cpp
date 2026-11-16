#include "Camera.h"

#include <GL/freeglut.h>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace
{
    constexpr float FREE_UNITS_PER_SECOND = 1.5f;
    constexpr float FREE_ACCELERATION = 9.0f;
    constexpr float FREE_BRAKING = 15.0f;
    constexpr float LOOK_RESPONSE = 22.0f;
    constexpr float ORBIT_RESPONSE = 12.0f;
    constexpr float ORBIT_VELOCITY_RESPONSE = 10.0f;
    constexpr float ORBIT_ROTATION_DEG_PER_SECOND = 90.0f;
    constexpr float ORBIT_HEIGHT_UNITS_PER_SECOND = 1.1f;
    constexpr float ORBIT_ZOOM_UNITS_PER_SECOND = 2.0f;
    constexpr float FREE_TURN_DEG_PER_SECOND = 100.0f;

    float SmoothFactor(float response, float dt)
    {
        return 1.0f - std::exp(-response * dt);
    }

    unsigned char NormalizedKey(unsigned char key)
    {
        return static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(key)));
    }

    int ArrowIndex(int key)
    {
        switch (key)
        {
        case GLUT_KEY_LEFT:  return 0;
        case GLUT_KEY_RIGHT: return 1;
        case GLUT_KEY_UP:    return 2;
        case GLUT_KEY_DOWN:  return 3;
        default: return -1;
        }
    }

    float Axis(bool positive, bool negative)
    {
        return static_cast<float>(positive) - static_cast<float>(negative);
    }
}

glm::vec3 CylinderPoint::At(const glm::vec3& center) const
{
    float a = glm::radians(angle);
    return center + glm::vec3(radius * std::cos(a), height, radius * std::sin(a));
}

Camera::Camera(int width, int height, glm::vec3 target, float sceneRadius)
    : width(width), height(height), target(target), sceneRadius(sceneRadius)
{
    orbit.angle = 90.0f;
    orbit.radius = sceneRadius * 3.0f;
    orbit.height = sceneRadius * 0.4f;
    nearPlane = sceneRadius * 0.01f;
    farPlane = sceneRadius * 50.0f;
    UpdatePosition();
}

void Camera::UpdatePosition()
{
    Position = orbit.At(target);
    Orientation = glm::normalize(target - Position);
    yaw = glm::degrees(std::atan2(Orientation.z, Orientation.x));
    pitch = glm::degrees(std::asin(std::clamp(Orientation.y, -1.0f, 1.0f)));
    targetYaw = yaw;
    targetPitch = pitch;
}

void Camera::UpdateOrientation()
{
    pitch = std::clamp(pitch, -89.0f, 89.0f);
    float y = glm::radians(yaw);
    float p = glm::radians(pitch);
    Orientation = glm::normalize(glm::vec3(
        std::cos(y) * std::cos(p), std::sin(p), std::sin(y) * std::cos(p)));
}

void Camera::ResetMotion()
{
    velocity = glm::vec3(0.0f);
    orbitAngleVelocity = 0.0f;
    orbitHeightVelocity = 0.0f;
    orbitZoomVelocity = 0.0f;

    // Release movement input on mode changes, but retain C/P state so key
    // repeat cannot switch modes back before the key is physically released.
    for (unsigned char key : {'w', 'a', 's', 'd', 'q', 'e', '+', '=', '-', '_'})
        heldKeys[key] = false;
    heldArrows.fill(false);
}

void Camera::ToggleMode()
{
    if (mode == ORBIT)
    {
        mode = FREE;
        // The current rendered orientation is retained without a jump.
        targetYaw = yaw;
        targetPitch = pitch;
    }
    else
    {
        mode = ORBIT;
        // Start orbit from the current free-camera azimuth. Moving out of the
        // globe and turning toward it happens smoothly in Update().
        glm::vec3 delta = Position - target;
        float horizontal = glm::length(glm::vec2(delta.x, delta.z));
        if (horizontal > 0.0001f)
            orbit.angle = glm::degrees(std::atan2(delta.z, delta.x));
        orbit.radius = std::clamp(horizontal, sceneRadius * minRadiusK, sceneRadius * maxRadiusK);
        orbit.height = std::clamp(delta.y, sceneRadius * minHeightK, sceneRadius * maxHeightK);
    }
    ResetMotion();
}

void Camera::MouseLook(int dx, int dy)
{
    if (mode != FREE) return;
    targetYaw = std::remainder(targetYaw + dx * mouseSensitivity, 360.0f);
    targetPitch = std::clamp(targetPitch - dy * mouseSensitivity, -89.0f, 89.0f);
}

void Camera::Update(float dt)
{
    if (dt <= 0.0f) return;
    dt = std::min(dt, 0.1f);

    if (mode == FREE)
    {
        // Mouse movement changes the target angle; rendered look follows
        // smoothly. Arrow keys can also aim while held.
        targetYaw = std::remainder(targetYaw +
            Axis(heldArrows[1], heldArrows[0]) * FREE_TURN_DEG_PER_SECOND * dt, 360.0f);
        targetPitch = std::clamp(targetPitch +
            Axis(heldArrows[2], heldArrows[3]) * FREE_TURN_DEG_PER_SECOND * dt,
            -89.0f, 89.0f);

        const float lookAlpha = SmoothFactor(LOOK_RESPONSE, dt);
        yaw = std::remainder(yaw + std::remainder(targetYaw - yaw, 360.0f) * lookAlpha, 360.0f);
        pitch += (targetPitch - pitch) * lookAlpha;
        UpdateOrientation();

        glm::vec3 right = glm::normalize(glm::cross(Orientation, Up));
        glm::vec3 direction = Orientation * Axis(heldKeys['w'], heldKeys['s'])
                            + right * Axis(heldKeys['d'], heldKeys['a'])
                            + Up * Axis(heldKeys['e'], heldKeys['q']);
        const float len = glm::length(direction);
        if (len > 0.0001f)
            direction /= len;     // Diagonal movement is not faster.

        glm::vec3 targetVelocity = direction * (sceneRadius * FREE_UNITS_PER_SECOND * moveSpeed);
        float response = len > 0.0001f ? FREE_ACCELERATION : FREE_BRAKING;
        velocity += (targetVelocity - velocity) * SmoothFactor(response, dt);
        Position += velocity * dt;
    }
    else
    {
        // Orbit angle, height and zoom all have smooth starts and stops.
        float angularTarget = Axis(heldArrows[1], heldArrows[0]) * ORBIT_ROTATION_DEG_PER_SECOND;
        float heightTarget = Axis(heldArrows[2], heldArrows[3]) * sceneRadius * ORBIT_HEIGHT_UNITS_PER_SECOND;
        float zoomTarget = Axis(heldKeys['-'] || heldKeys['_'], heldKeys['+'] || heldKeys['='])
                         * sceneRadius * ORBIT_ZOOM_UNITS_PER_SECOND;
        float alpha = SmoothFactor(ORBIT_VELOCITY_RESPONSE, dt);
        orbitAngleVelocity += (angularTarget - orbitAngleVelocity) * alpha;
        orbitHeightVelocity += (heightTarget - orbitHeightVelocity) * alpha;
        orbitZoomVelocity += (zoomTarget - orbitZoomVelocity) * alpha;

        orbit.angle = std::remainder(orbit.angle + orbitAngleVelocity * dt, 360.0f);
        float nextHeight = std::clamp(orbit.height + orbitHeightVelocity * dt,
            sceneRadius * minHeightK, sceneRadius * maxHeightK);
        if (nextHeight != orbit.height + orbitHeightVelocity * dt) orbitHeightVelocity = 0.0f;
        orbit.height = nextHeight;
        float nextRadius = std::clamp(orbit.radius + orbitZoomVelocity * dt,
            sceneRadius * minRadiusK, sceneRadius * maxRadiusK);
        if (nextRadius != orbit.radius + orbitZoomVelocity * dt) orbitZoomVelocity = 0.0f;
        orbit.radius = nextRadius;

        Position += (orbit.At(target) - Position) * SmoothFactor(ORBIT_RESPONSE, dt);

        glm::vec3 desiredDirection = glm::normalize(target - Position);
        targetYaw = glm::degrees(std::atan2(desiredDirection.z, desiredDirection.x));
        targetPitch = glm::degrees(std::asin(std::clamp(desiredDirection.y, -1.0f, 1.0f)));
        const float lookAlpha = SmoothFactor(LOOK_RESPONSE, dt);
        yaw = std::remainder(yaw + std::remainder(targetYaw - yaw, 360.0f) * lookAlpha, 360.0f);
        pitch += (targetPitch - pitch) * lookAlpha;
        UpdateOrientation();
    }
}

void Camera::ClampToSphere(const glm::vec3& center, float softRadius, float easeSpeed, float dt)
{
    // Orbit already has its own radius and height limits. This soft limit is
    // only for the free-fly camera.
    if (mode != FREE || dt <= 0.0f || easeSpeed <= 0.0f) return;
    glm::vec3 delta = Position - center;
    float dist = glm::length(delta);
    if (dist <= softRadius || dist < 0.0001f) return;
    float easedDist = softRadius + (dist - softRadius) * std::exp(-easeSpeed * dt);
    Position = center + (delta / dist) * easedDist;
}

glm::mat4 Camera::ViewMatrix() const
{
    return glm::lookAt(Position, Position + Orientation, Up);
}

glm::mat4 Camera::ProjectionMatrix() const
{
    float aspect = static_cast<float>(width) / static_cast<float>(height > 0 ? height : 1);
    if (perspective)
        return glm::perspective(glm::radians(fovDeg), aspect, nearPlane, farPlane);

    float h = sceneRadius * 1.2f;
    float w = h * aspect;
    return glm::ortho(-w, w, -h, h, nearPlane, farPlane);
}

void Camera::Matrix(float FOVdeg, float nearPl, float farPl, Shader& shader, const char* uniform)
{
    fovDeg = FOVdeg;
    nearPlane = nearPl;
    farPlane = farPl;
    glm::mat4 m = ProjectionMatrix() * ViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, uniform), 1, GL_FALSE, glm::value_ptr(m));
}

bool Camera::Inputs(unsigned char key)
{
    unsigned char k = NormalizedKey(key);
    bool recognized = (k == 'c' || k == 'p' || k == 'w' || k == 'a' || k == 's' ||
        k == 'd' || k == 'q' || k == 'e' || key == '+' || key == '=' ||
        key == '-' || key == '_');
    if (!recognized) return false;
    if (mode == ORBIT && k != 'c' && k != 'p' &&
        key != '+' && key != '=' && key != '-' && key != '_') return false;

    if (heldKeys[k]) return true; // Ignore keyboard auto-repeat.
    heldKeys[k] = true;

    if (k == 'c') ToggleMode();
    else if (k == 'p') perspective = !perspective;
    else if (mode == FREE)
    {
        if (key == '+' || key == '=') moveSpeed = std::min(moveSpeed * 1.25f, 20.0f);
        else if (key == '-' || key == '_') moveSpeed = std::max(moveSpeed * 0.8f, 0.05f);
    }
    return true;
}

void Camera::KeyUp(unsigned char key)
{
    heldKeys[NormalizedKey(key)] = false;
}

bool Camera::SpecialInputs(int key)
{
    int i = ArrowIndex(key);
    if (i < 0) return false;
    heldArrows[i] = true;
    return true;
}

void Camera::SpecialUp(int key)
{
    int i = ArrowIndex(key);
    if (i >= 0) heldArrows[i] = false;
}

void Camera::Resize(int newWidth, int newHeight)
{
    width = newWidth;
    height = newHeight;
}
