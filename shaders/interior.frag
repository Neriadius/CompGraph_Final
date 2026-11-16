#version 330 core

// Per-fragment Blinn-Phong for the interior (Person 3), used by two programs:
//   interior.vert + interior.frag : procedural geometry (colour + material per vertex)
//   model.vert    + interior.frag : imported OBJ models (colour + material per part, texture)
//
// Lighting:
//   moon (directional) + up to MAX_POINT_LIGHTS point lights (campfire, hut windows, lantern)
//   + hemisphere ambient + emission (lit windows, glowing coals)
// Procedural surface detail (bark, tree rings, stone, snow glitter) for the built mesh,
// pattern id from the vertex (see src/MeshBuilder.h); imported models use their textures.

#define MAX_POINT_LIGHTS 6

in vec3 vWorldPos;
in vec3 vLocalPos;
in vec3 vNormal;
in vec3 vColor;
in vec2 vUV;
flat in vec4 vMat;
flat in vec2 vEmit;

out vec4 FragColor;

uniform vec3 camPos;
uniform float time;

// ---- light block (same names as src/SceneLights.h, reusable in other shaders) ----
uniform vec3 moonDir;                          // direction TOWARDS the key light (moon / sun)
uniform vec3 moonColor;
uniform float daylight;                        // 0 = night, 1 = day
uniform vec3 skyAmbient;
uniform vec3 groundAmbient;
uniform int pointCount;
uniform vec3 pointPos[MAX_POINT_LIGHTS];
uniform vec3 pointColor[MAX_POINT_LIGHTS];     // colour * intensity
uniform vec2 pointFalloff[MAX_POINT_LIGHTS];   // x = soft radius r0, y = range

// ---- emission ----
uniform float windowGlow;                      // 0..1, hut lights (L)
uniform float fireGlow;                        // 0..~1.3, campfire flicker (F)
uniform vec3 windowEmitColor;
uniform vec3 fireEmitColor;

// ---- imported models (src/Model.cpp); stay 0 for the procedural mesh ----
uniform int useTexture;                        // albedo = colour * texture
uniform int alphaCutout;                       // discard where texture alpha < 0.5 (branches)
uniform int twoSided;                          // flip the normal on back faces (thin cards, cloth)
uniform sampler2D albedoMap;

// ---------------------------------------------------------------
// Hash without sine (Dave Hoskins, MIT) + value noise.
// Not called noise2/noise3: those names are GLSL built-ins and strict drivers reject a redefinition.
// ---------------------------------------------------------------
float hash12(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}

float hash13(vec3 p3)
{
	p3 = fract(p3 * 0.1031);
	p3 += dot(p3, p3.zyx + 31.32);
	return fract((p3.x + p3.y) * p3.z);
}

vec3 hash33(vec3 p3)
{
	p3 = fract(p3 * vec3(0.1031, 0.1030, 0.0973));
	p3 += dot(p3, p3.yxz + 33.33);
	return fract((p3.xxy + p3.yxx) * p3.zyx);
}

float valueNoise2(vec2 p)
{
	vec2 i = floor(p), f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash12(i), hash12(i + vec2(1, 0)), f.x),
	           mix(hash12(i + vec2(0, 1)), hash12(i + vec2(1, 1)), f.x), f.y);
}

float valueNoise3(vec3 p)
{
	vec3 i = floor(p), f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix(hash13(i), hash13(i + vec3(1, 0, 0)), f.x),
	               mix(hash13(i + vec3(0, 1, 0)), hash13(i + vec3(1, 1, 0)), f.x), f.y),
	           mix(mix(hash13(i + vec3(0, 0, 1)), hash13(i + vec3(1, 0, 1)), f.x),
	               mix(hash13(i + vec3(0, 1, 1)), hash13(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

float fbm3(vec3 p)
{
	return 0.5 * valueNoise3(p) + 0.3 * valueNoise3(p * 2.1) + 0.2 * valueNoise3(p * 4.3);
}

// ---------------------------------------------------------------
// Procedural patterns: albedo change for each pattern id
// ---------------------------------------------------------------
vec3 Pattern(vec3 base, int id, vec2 uv, vec3 lp)
{
	if (id == 1)          // bark: grain along the log
	{
		float g = valueNoise2(vec2(uv.x * 90.0, uv.y * 7.0));
		float fine = valueNoise2(vec2(uv.x * 300.0, uv.y * 25.0));
		float knot = smoothstep(0.80, 0.92, valueNoise2(uv * vec2(40.0, 18.0)));
		return base * (0.70 + 0.45 * g) * (0.88 + 0.24 * fine) * (1.0 - 0.45 * knot);
	}
	if (id == 2)          // tree rings on cut ends
	{
		float r = length(uv) + 0.004 * valueNoise2(uv * 90.0);
		float ring = fract(r * 240.0);
		float line = smoothstep(0.0, 0.18, ring) * smoothstep(1.0, 0.55, ring);
		return base * mix(0.62, 1.05, line) * (0.85 + 0.15 * smoothstep(0.0, 0.01, r));
	}
	if (id == 3)          // snow: soft drifts of tone
	{
		return base * (0.93 + 0.09 * valueNoise3(lp * 22.0));
	}
	if (id == 4)          // stone
	{
		float m = fbm3(lp * 45.0);
		float speck = step(0.82, valueNoise3(lp * 260.0));
		return base * (0.62 + 0.62 * m) + vec3(0.08) * speck;
	}
	return base;
}

// Point light falloff (see src/SceneLights.h)
float Attenuation(float d, vec2 falloff)
{
	float soft = 1.0 / (1.0 + (d * d) / (falloff.x * falloff.x));
	float w = clamp(1.0 - pow(d / falloff.y, 4.0), 0.0, 1.0);
	return soft * w * w;
}

void main()
{
	float specStrength = vMat.x;
	float shininess = vMat.y;
	int pattern = int(vMat.z + 0.5);
	float amount = vMat.w;

	vec3 base = vColor;
	if (useTexture == 1)
	{
		vec4 tex = texture(albedoMap, vUV);
		if (alphaCutout == 1 && tex.a < 0.5) discard;
		base *= tex.rgb;
	}
	vec3 albedo = mix(base, Pattern(base, pattern, vUV, vLocalPos), amount);

	vec3 N = normalize(vNormal);
	if (twoSided == 1 && !gl_FrontFacing) N = -N;
	vec3 V = normalize(camPos - vWorldPos);

	// terrain: snow where the ground is gentle (and higher up), bare rock on steep faces;
	// mountain shadows were baked per vertex: uv.x = moon, uv.y = sun
	float snowCover = (pattern == 3) ? 1.0 : 0.0;
	float keyVisible = 1.0;
	if (pattern == 5)
	{
		float n = valueNoise3(vLocalPos * 16.0);
		float height = vLocalPos.y + 0.55;
		snowCover = smoothstep(0.42, 0.68, N.y + 0.20 * (n - 0.5) + 0.32 * height);
		vec3 snow = base * (0.93 + 0.09 * valueNoise3(vLocalPos * 22.0));
		float strata = 0.85 + 0.15 * sin(vLocalPos.y * 160.0 + 6.0 * n);
		vec3 rock = vec3(0.36, 0.33, 0.31) * (0.6 + 0.7 * fbm3(vLocalPos * 38.0)) * strata;
		albedo = mix(rock, snow, snowCover);
		keyVisible = mix(vUV.x, vUV.y, daylight);
	}
	float wrap = 0.35 * snowCover;                     // snow scatters light a little past the terminator

	// hemisphere ambient: sky from above, dark snow bounce from below
	vec3 ambient = mix(groundAmbient, skyAmbient, 0.5 + 0.5 * N.y);
	vec3 diffuse = vec3(0.0);
	vec3 specular = vec3(0.0);

	// moon
	{
		vec3 L = normalize(moonDir);
		vec3 H = normalize(L + V);
		float ndl = max((dot(N, L) + wrap) / (1.0 + wrap), 0.0);
		diffuse += moonColor * ndl * keyVisible;
		specular += moonColor * specStrength * pow(max(dot(N, H), 0.0), shininess) * step(0.0, dot(N, L)) * keyVisible;
	}

	// cool sky fill from the side opposite the moon, so faces turned away from it still read
	{
		vec3 F = normalize(vec3(-moonDir.x, 0.35, -moonDir.z));
		diffuse += skyAmbient * 0.8 * max(dot(N, F), 0.0);
	}

	// point lights
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		if (i >= pointCount) break;
		vec3 toL = pointPos[i] - vWorldPos;
		float d = length(toL);
		vec3 L = toL / max(d, 1e-4);
		vec3 H = normalize(L + V);
		float att = Attenuation(d, pointFalloff[i]);
		float ndl = max((dot(N, L) + wrap) / (1.0 + wrap), 0.0);
		diffuse += pointColor[i] * ndl * att;
		specular += pointColor[i] * att * specStrength * pow(max(dot(N, H), 0.0), shininess) * step(0.0, dot(N, L));
	}

	// snow glitter: random tiny facets catch the moon / fire as the camera moves
	if (snowCover > 0.0)
	{
		vec3 cell = floor(vLocalPos * 260.0);
		vec3 rnd = hash33(cell);
		if (rnd.x > 0.55)
		{
			vec3 gN = normalize(N + (rnd - 0.5) * 1.4);
			vec3 H = normalize(normalize(moonDir) + V);
			float g = pow(max(dot(gN, H), 0.0), 220.0) * 2.5 * keyVisible;
			if (pointCount > 0)
			{
				vec3 toL = pointPos[0] - vWorldPos;
				vec3 Hf = normalize(normalize(toL) + V);
				g += pow(max(dot(gN, Hf), 0.0), 220.0) * 2.5 * Attenuation(length(toL), pointFalloff[0])
				     * length(pointColor[0]);
			}
			specular += vec3(0.9, 0.95, 1.0) * g * snowCover;
		}
	}

	vec3 emission = vEmit.x * windowGlow * windowEmitColor * (0.85 + 0.3 * valueNoise2(vLocalPos.xy * 60.0 + vLocalPos.zz * 60.0))
	              + vEmit.y * fireGlow * fireEmitColor * mix(0.25, 1.7, valueNoise3(vLocalPos * 150.0 + vec3(0.0, -time * 0.9, time * 0.4)));

	vec3 color = albedo * (ambient + diffuse) + specular + emission;
	FragColor = vec4(color, 1.0);
}
