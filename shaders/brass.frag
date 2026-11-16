#version 330 core

in vec3 vLocalPos;
in vec3 vLocalNormal;
in vec3 vWorldPos;
out vec4 FragColor;

uniform mat4 model;
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


// value noise
float hash(vec3 p)
{
	p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
	p *= 17.0;
	return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 x)
{
	vec3 i = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);
	return mix(
		mix(mix(hash(i + vec3(0, 0, 0)), hash(i + vec3(1, 0, 0)), f.x),
		    mix(hash(i + vec3(0, 1, 0)), hash(i + vec3(1, 1, 0)), f.x), f.y),
		mix(mix(hash(i + vec3(0, 0, 1)), hash(i + vec3(1, 0, 1)), f.x),
		    mix(hash(i + vec3(0, 1, 1)), hash(i + vec3(1, 1, 1)), f.x), f.y),
		f.z);
}

float fbm(vec3 p)
{
	float v = 0.0, a = 0.5;
	for (int i = 0; i < 4; i++)
	{
		v += a * noise(p);
		p *= 2.03;
		a *= 0.5;
	}
	return v;
}

// Brushed grain 0..1. Flat faces: lines along X. Round parts: lines along the shaft (Z).
float grain(vec3 p, vec3 n)
{
	if (abs(n.z) > 0.7)
		return noise(vec3(p.x * 6.0, p.y * 110.0, 3.7));
	return noise(vec3(p.x * 110.0, p.y * 110.0, p.z * 6.0));
}

void main()
{
	vec3 p = vLocalPos;
	vec3 nl = normalize(vLocalNormal);

	// bump from the grain (computed in the key's own space)
	float g = grain(p, nl);
	const float e = 0.002;
	vec3 grad = vec3(grain(p + vec3(e, 0, 0), nl) - g,
	                 grain(p + vec3(0, e, 0), nl) - g,
	                 grain(p + vec3(0, 0, e), nl) - g) / e;
	grad -= nl * dot(grad, nl);                       // only the part along the surface
	nl = normalize(nl - 0.0004 * grad);

	vec3 n = normalize(mat3(transpose(inverse(model))) * nl);
	vec3 v = normalize(camPos - vWorldPos);

	// colour
	float blotch = fbm(p * 9.0);
	float tone = clamp(0.40 * g + 0.60 * blotch, 0.0, 1.0);
	vec3 brassDark  = vec3(0.80, 0.55, 0.12);
	vec3 brassLight = vec3(1.00, 0.84, 0.36);
	vec3 base = mix(brassDark, brassLight, smoothstep(0.25, 0.85, tone));

	float tarnish = smoothstep(0.64, 0.82, fbm(p * 5.0 + vec3(7.3)));
	base = mix(base, vec3(0.34, 0.25, 0.10), tarnish * 0.45);

	float scratch = smoothstep(0.93, 0.99, noise(vec3(p.x * 5.0, p.y * 420.0, 1.3)))
	              + smoothstep(0.94, 0.99, noise(vec3(p.x * 150.0, p.y * 150.0, p.z * 3.0)));
	base += vec3(0.30, 0.26, 0.15) * scratch * 0.35;

	// Shared light colours and flickering fire/window point lights.
	float shininess = mix(90.0, 25.0, tarnish);   // tarnished = duller
	vec3 lighting = sceneLighting(n, vWorldPos, shininess, 1.25);

	// A subtle procedural reflection helps brass stay metallic in the dark.
	vec3 r = reflect(-v, n);
	vec3 env = mix(vec3(0.10, 0.07, 0.04), vec3(0.85, 0.90, 1.00), smoothstep(-0.3, 0.6, r.y));
	float fresnel = 0.55 + 0.45 * pow(1.0 - max(dot(n, v), 0.0), 3.0);
	vec3 lit = base * lighting * 0.65
	         + base * env * fresnel * (1.0 - tarnish * 0.6) * 0.45;

	FragColor = vec4(lit, 1.0);
}
