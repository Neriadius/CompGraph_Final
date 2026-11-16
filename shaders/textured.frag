#version 330 core

in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D tex0;
uniform vec3 camPos;
#define MAX_POINT_LIGHTS 6
uniform vec3 moonDir, moonColor, skyAmbient, groundAmbient;
uniform int pointCount;
uniform vec3 pointPos[MAX_POINT_LIGHTS], pointColor[MAX_POINT_LIGHTS];
uniform vec2 pointFalloff[MAX_POINT_LIGHTS];

vec3 ambientLight(vec3 n)
{
    return mix(groundAmbient, skyAmbient, clamp(n.y * 0.5 + 0.5, 0.0, 1.0));
}

vec3 directLight(vec3 n, vec3 v, vec3 l, vec3 color, float shininess, float specStrength)
{
    float ndotl = max(dot(n, l), 0.0);
    vec3 h = normalize(l + v);
    float spec = ndotl > 0.0 ? pow(max(dot(n, h), 0.0), shininess) : 0.0;
    return color * (ndotl + specStrength * spec);
}

vec3 sceneLighting(vec3 n, vec3 worldPos, float shininess, float specStrength)
{
    vec3 v = normalize(camPos - worldPos);
    vec3 light = ambientLight(n) + directLight(n, v, normalize(moonDir), moonColor, shininess, specStrength);
    for (int i = 0; i < pointCount && i < MAX_POINT_LIGHTS; ++i)
    {
        vec3 delta = pointPos[i] - worldPos;
        float d = length(delta);
        float r0 = max(pointFalloff[i].x, 0.001);
        float range = max(pointFalloff[i].y, 0.001);
        float fade = pow(clamp(1.0 - pow(d / range, 4.0), 0.0, 1.0), 2.0);
        float attenuation = fade / (1.0 + pow(d / r0, 2.0));
        if (d > 0.0001)
            light += attenuation * directLight(n, v, delta / d, pointColor[i], shininess, specStrength);
    }
    return light;
}


void main()
{
	vec3 albedo = texture(tex0, vUV).rgb;
	vec3 n = normalize(vNormal);
	FragColor = vec4(albedo * sceneLighting(n, vWorldPos, 32.0, 0.18), 1.0);
}
