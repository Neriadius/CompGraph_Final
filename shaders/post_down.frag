#version 330 core
in vec2 vUv;
out vec4 FragColor;

uniform sampler2D source;
uniform vec2 texel;

void main()
{
	vec3 c = texture(source, vUv + texel * vec2(-1.0, -1.0)).rgb
	       + texture(source, vUv + texel * vec2( 1.0, -1.0)).rgb
	       + texture(source, vUv + texel * vec2(-1.0,  1.0)).rgb
	       + texture(source, vUv + texel * vec2( 1.0,  1.0)).rgb;
	FragColor = vec4(c * 0.25, 1.0);
}
