#version 330 core
// One direction of a 9-tap Gaussian blur, done with 5 bilinear taps
in vec2 vUv;
out vec4 FragColor;

uniform sampler2D source;
uniform vec2 blurStep;   // direction * tap spacing, in texture coordinates

const float OFFSET[3] = float[](0.0, 1.3846153846, 3.2307692308);
const float WEIGHT[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);

void main()
{
	vec3 c = texture(source, vUv).rgb * WEIGHT[0];
	for (int i = 1; i < 3; i++)
	{
		c += texture(source, vUv + blurStep * OFFSET[i]).rgb * WEIGHT[i];
		c += texture(source, vUv - blurStep * OFFSET[i]).rgb * WEIGHT[i];
	}
	FragColor = vec4(c, 1.0);
}
