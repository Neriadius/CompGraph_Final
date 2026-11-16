#version 330 core
// Menu background
in vec2 vUv;
out vec4 FragColor;

uniform sampler2D sharp;
uniform sampler2D blurred;
uniform float amount;    // 0 = normal picture, 1 = full menu blur
uniform float time;

void main()
{
	vec3 a = texture(sharp, vUv).rgb;
	vec3 b = texture(blurred, vUv).rgb;
	vec3 c = mix(a, b, smoothstep(0.0, 0.6, amount));

	vec2 d = vUv - 0.5;
	float vignette = 1.0 - 0.55 * dot(d, d) * 2.0;
	vec3 menuLook = c * vec3(0.62, 0.66, 0.74) * vignette;
	c = mix(c, menuLook, amount);

	float n = fract(sin(dot(gl_FragCoord.xy + fract(time) * 61.0, vec2(12.9898, 78.233))) * 43758.5453);
	c += (n - 0.5) / 255.0 * amount;
	FragColor = vec4(c, 1.0);
}
