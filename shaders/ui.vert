#version 330 core
// 2D menu / overlay geometry in window pixels
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUv;

uniform vec2 screen;   // window size in pixels
uniform vec2 origin;   // where the element starts on screen
uniform vec2 scale;    // text: size in pixels; rectangles: size plus padding
uniform vec2 offset;   // rectangles: -padding

out vec2 vUv;
out vec2 vLocal;       // pixels relative to the element's top-left corner

void main()
{
	vLocal = aPos * scale + offset;
	vUv = aUv;
	vec2 pixel = origin + vLocal;
	gl_Position = vec4(pixel.x / screen.x * 2.0 - 1.0, 1.0 - pixel.y / screen.y * 2.0, 0.0, 1.0);
}
