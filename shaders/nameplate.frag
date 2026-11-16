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

// Brushed brass: diffuse engraving + bright, narrow Blinn-Phong highlights.
void main()
{
    vec3 n = normalize(vNormal);
    vec3 viewDir = normalize(camPos - vWorldPos);
    vec3 pigment = texture(tex0, vUV).rgb;
    vec3 light = mix(groundAmbient, skyAmbient, clamp(n.y * 0.5 + 0.5, 0.0, 1.0));
    float shine = 0.0;

    vec3 l = normalize(moonDir);
    float diffuse = max(dot(n, l), 0.0);
    light += moonColor * diffuse;
    if (diffuse > 0.0)
        shine += pow(max(dot(n, normalize(l + viewDir)), 0.0), 90.0) *
                 dot(moonColor, vec3(0.25, 0.65, 0.10));

    for (int i = 0; i < pointCount && i < MAX_POINT_LIGHTS; ++i)
    {
        vec3 delta = pointPos[i] - vWorldPos;
        float d = length(delta);
        if (d < 0.0001) continue;
        l = delta / d;
        float r0 = max(pointFalloff[i].x, 0.001);
        float range = max(pointFalloff[i].y, 0.001);
        float fade = pow(clamp(1.0 - pow(d / range, 4.0), 0.0, 1.0), 2.0);
        float attenuation = fade / (1.0 + pow(d / r0, 2.0));
        diffuse = max(dot(n, l), 0.0);
        light += pointColor[i] * (diffuse * attenuation);
        if (diffuse > 0.0)
            shine += attenuation * pow(max(dot(n, normalize(l + viewDir)), 0.0), 90.0) *
                     dot(pointColor[i], vec3(0.25, 0.65, 0.10));
    }

    // Specular glint stays metallic even over the dark stamped letters.
    vec3 color = pigment * light + vec3(1.0, 0.84, 0.44) * shine * 0.55;
    FragColor = vec4(color, 1.0);
}
