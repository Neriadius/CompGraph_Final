#version 330 core
// Signed distance field text
in vec2 vUv;
out vec4 FragColor;

uniform sampler2D atlas;
uniform vec4 color;
uniform float pxPerUnit;  // screen pixels per 1.0 of atlas value
uniform float edgePx;     // > 0 grows the letters (used for the glow pass)
uniform float softPx;     // 1 = sharp edge, larger = soft halo

void main()
{
	float d = (texture(atlas, vUv).r - 0.5) * pxPerUnit;
	float a = clamp((d + edgePx) / softPx + 0.5, 0.0, 1.0);
	if (softPx > 1.5) a = a * a * (3.0 - 2.0 * a);
	FragColor = vec4(color.rgb, color.a * a);
}
