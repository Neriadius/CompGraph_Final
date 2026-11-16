#version 330 core
layout (location = 0) in vec2 screenPosition;
void main()
{
    gl_Position = vec4(screenPosition, 0.0, 1.0);
}
