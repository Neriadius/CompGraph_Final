#version 330 core
in vec2 vLocal;
out vec4 FragColor;

uniform vec2 size;
uniform float radius;
uniform vec4 fill;
uniform vec4 border;
uniform float borderPx;
uniform float softPx;

float roundedBox(vec2 p, vec2 halfSize, float r)
{
	vec2 q = abs(p) - halfSize + r;
	return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main()
{
	float d = roundedBox(vLocal - 0.5 * size, 0.5 * size, radius);   // < 0 inside
	float shape = clamp(0.5 - d / softPx, 0.0, 1.0);
	if (softPx > 1.5) shape = shape * shape * (3.0 - 2.0 * shape);
	float inner = clamp(0.5 - (d + borderPx), 0.0, 1.0);             // 1 inside the outline
	vec4 color = borderPx > 0.0 ? mix(border, fill, inner) : fill;
	FragColor = vec4(color.rgb, color.a * shape);
}
