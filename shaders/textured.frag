#version 330 core

in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D tex0;
uniform vec3 lightDir;       // direction TOWARDS the light (world space)
uniform vec3 lightColor;
uniform vec3 ambientColor;

void main()
{
	vec3 albedo = texture(tex0, vUV).rgb;
	vec3 n = normalize(vNormal);
	float diff = max(dot(n, normalize(lightDir)), 0.0);
	FragColor = vec4(albedo * (ambientColor + lightColor * diff), 1.0);
}
