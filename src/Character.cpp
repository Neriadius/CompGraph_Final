#include "Character.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "InteriorModels.h"

namespace
{
	// Pivots (character space: origin on the ground under the seat, facing +Z)
	const glm::vec3 HIPS(0.0f, 0.060f, -0.004f);
	const glm::vec3 NECK(0.0f, 0.100f, 0.002f);        // relative to HIPS
	const glm::vec3 SHOULDER(0.036f, 0.078f, 0.0f);    // relative to HIPS, x mirrored per side
	const glm::vec3 ELBOW(0.010f, -0.036f, 0.024f);    // relative to SHOULDER
	const glm::vec3 WRIST(-0.012f, 0.012f, 0.040f);    // relative to ELBOW

	const glm::vec3 X(1, 0, 0), Y(0, 1, 0);

	Material Coat() { return palette::Plain({ 0.62f, 0.13f, 0.12f }, 0.10f, 14.0f); }
	Material Knit(const glm::vec3& c)
	{
		Material m = palette::Plain(c, 0.04f, 8.0f);
		m.pattern = PAT_KNIT; m.patternAmount = 1.0f;
		return m;
	}
	const glm::vec3 MUSTARD(0.86f, 0.60f, 0.16f), CREAM(0.93f, 0.89f, 0.80f);

	glm::mat4 T(const glm::vec3& p) { return glm::translate(glm::mat4(1.0f), p); }
	glm::mat4 Ellipsoid(const glm::vec3& c, const glm::vec3& r) { return glm::scale(T(c), r); }

	// cylinder between two points with round joints at both ends
	void Limb(MeshBuilder& mb, glm::vec3 a, glm::vec3 b, float r, const Material& m)
	{
		float len;
		const glm::mat4 frame = AlignY(a, b, &len);          // sets len before it is used
		mb.Cylinder(frame, r, r * 0.95f, len, 10, m);
		mb.Sphere(T(a), r, 5, 10, m);
		mb.Sphere(T(b), r * 0.95f, 5, 10, m);
	}
}

void Character::Init()
{
	const Material pants = palette::Plain({ 0.17f, 0.21f, 0.33f }, 0.05f, 10.0f);
	const Material boot = palette::Plain({ 0.24f, 0.15f, 0.10f }, 0.25f, 24.0f);
	const Material fur = Knit(CREAM);

	// ---- body: hips + legs + boots (static, in root space) ----
	{
		MeshBuilder mb;
		mb.Sphere(Ellipsoid(HIPS, { 0.034f, 0.016f, 0.028f }), 1.0f, 6, 14, pants);
		for (int s = -1; s <= 1; s += 2)
		{
			glm::vec3 hip(s * 0.017f, 0.060f, 0.0f), knee(s * 0.021f, 0.062f, 0.058f), ankle(s * 0.024f, 0.016f, 0.066f);
			Limb(mb, hip, knee, 0.0135f, pants);
			Limb(mb, knee, ankle, 0.0120f, pants);
			mb.Sphere(Ellipsoid(ankle + glm::vec3(0, -0.006f, 0.007f), { 0.0135f, 0.0105f, 0.021f }), 1.0f, 6, 12, boot);
			mb.Torus(T(ankle + glm::vec3(0, 0.002f, 0)), 0.0122f, 0.0042f, 14, 6, fur);
		}
		body.Upload(mb);
	}

	// ---- torso: coat, buttons, hood, scarf (around the hip pivot) ----
	{
		MeshBuilder mb;
		const std::vector<glm::vec2> coat = {
			{ 0.000f, -0.010f }, { 0.036f, -0.008f }, { 0.041f, 0.004f }, { 0.040f, 0.030f },
			{ 0.036f, 0.060f }, { 0.031f, 0.080f }, { 0.023f, 0.095f }, { 0.011f, 0.102f }, { 0.0f, 0.104f } };
		mb.Lathe(glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.85f)), coat, 18, Coat());

		Material toggle = palette::Plain({ 0.88f, 0.82f, 0.68f }, 0.3f, 30.0f);
		for (int k = 0; k < 3; k++)
		{
			float y = 0.018f + k * 0.021f;
			float r = 0.0f;                                   // coat radius at y (from the profile)
			for (size_t p = 1; p < coat.size(); p++)
				if (y >= coat[p - 1].y && y <= coat[p].y)
					r = glm::mix(coat[p - 1].x, coat[p].x, (y - coat[p - 1].y) / (coat[p].y - coat[p - 1].y));
			mb.Sphere(Ellipsoid({ 0.0f, y, 0.85f * r + 0.0008f }, { 0.0045f, 0.0028f, 0.0025f }), 1.0f, 4, 8, toggle);
		}
		Material hood = Coat();
		hood.color *= 0.8f;
		mb.Sphere(Ellipsoid({ 0.0f, 0.088f, -0.024f }, { 0.022f, 0.015f, 0.013f }), 1.0f, 6, 12, hood);

		// scarf: ring around the neck, one tail on the chest, one down the back
		Material scarf = Knit(MUSTARD);
		mb.Torus(T({ 0.0f, 0.094f, 0.001f }), 0.021f, 0.0078f, 18, 8, scarf);
		glm::mat4 front = glm::rotate(T({ 0.013f, 0.072f, 0.033f }), 0.25f, glm::vec3(0, 0, 1));
		front = glm::rotate(front, -0.25f, X);
		mb.Box(front, { 0.0065f, 0.019f, 0.0028f }, scarf);
		glm::mat4 back = glm::rotate(T({ -0.010f, 0.066f, -0.031f }), -0.15f, glm::vec3(0, 0, 1));
		back = glm::rotate(back, 0.3f, X);
		mb.Box(back, { 0.0065f, 0.023f, 0.0028f }, scarf);
		for (int k = -1; k <= 1; k += 2)
		{
			mb.Box(glm::translate(front, { 0, k * 0.009f, 0.0003f }), { 0.0067f, 0.0022f, 0.0028f }, Knit(CREAM));
			mb.Box(glm::translate(back, { 0, k * 0.011f, -0.0003f }), { 0.0067f, 0.0022f, 0.0028f }, Knit(CREAM));
		}
		torso.Upload(mb);
	}

	// ---- head: face, hair, beanie with pom-pom (around the neck pivot) ----
	{
		MeshBuilder mb;
		const Material skin = palette::Plain({ 0.93f, 0.74f, 0.62f }, 0.12f, 20.0f);
		mb.Sphere(T({ 0.0f, 0.030f, 0.0f }), 0.032f, 10, 18, skin);
		mb.Sphere(Ellipsoid({ 0.0f, 0.029f, -0.006f }, { 0.0336f, 0.030f, 0.032f }), 1.0f, 8, 16,
			palette::Plain({ 0.30f, 0.18f, 0.10f }, 0.15f, 20.0f));
		const Material eye = palette::Plain({ 0.04f, 0.03f, 0.03f }, 1.0f, 120.0f);
		const Material cheek = palette::Plain({ 0.98f, 0.55f, 0.52f }, 0.1f, 10.0f);
		for (int s = -1; s <= 1; s += 2)
		{
			mb.Sphere(T({ s * 0.0115f, 0.029f, 0.0283f }), 0.0036f, 5, 8, eye);
			mb.Sphere(Ellipsoid({ s * 0.0195f, 0.019f, 0.0245f }, { 0.0068f, 0.0045f, 0.0035f }), 1.0f, 5, 8, cheek);
		}
		mb.Sphere(T({ 0.0f, 0.022f, 0.0318f }), 0.0052f, 5, 8, palette::Plain({ 0.95f, 0.58f, 0.50f }, 0.15f, 20.0f));
		// little smile: a thin dark arc
		mb.Torus(glm::rotate(T({ 0.0f, 0.0165f, 0.0262f }), 1.35f, X), 0.0055f, 0.0009f, 10, 4,
			palette::Plain({ 0.35f, 0.12f, 0.10f }));

		Material hat = Knit(MUSTARD);
		mb.Sphere(Ellipsoid({ 0.0f, 0.040f, -0.002f }, { 0.0345f, 0.034f, 0.0345f }), 1.0f, 8, 18, hat, 0.0f, 1.0f);
		mb.Torus(T({ 0.0f, 0.041f, -0.002f }), 0.0335f, 0.0058f, 22, 8, Knit(CREAM));
		mb.Sphere(T({ 0.0f, 0.077f, -0.004f }), 0.012f, 7, 12, Knit(CREAM));
		head.Upload(mb);
	}

	// ---- arms (around shoulder / elbow pivots), mirrored for each side ----
	for (int i = 0; i < 2; i++)
	{
		const float s = i ? 1.0f : -1.0f;
		const glm::vec3 elbow(s * ELBOW.x, ELBOW.y, ELBOW.z);
		const glm::vec3 wrist(s * WRIST.x, WRIST.y, WRIST.z);

		MeshBuilder upper;
		Limb(upper, glm::vec3(0.0f), elbow, 0.0118f, Coat());
		upperArm[i].Upload(upper);

		MeshBuilder fore;
		Limb(fore, glm::vec3(0.0f), wrist * 0.85f, 0.0108f, Coat());
		fore.Torus(AlignY(wrist * 0.8f, wrist), 0.0105f, 0.0038f, 14, 6, fur);       // cuff
		Material mitten = Knit(MUSTARD);
		glm::vec3 hand = wrist + glm::normalize(wrist) * 0.010f;
		fore.Sphere(glm::scale(AlignY(hand, hand + glm::normalize(wrist) * 0.01f), glm::vec3(0.95f, 1.15f, 0.8f)),
			0.0118f, 6, 12, mitten);
		fore.Sphere(T(hand + glm::vec3(-s * 0.009f, 0.004f, -0.002f)), 0.0048f, 4, 8, mitten);   // thumb
		forearm[i].Upload(fore);
	}
}

void Character::Draw(const glm::mat4& root, float t, const std::function<void(const glm::mat4&)>& setModel) const
{
	setModel(root);
	body.Draw();

	// torso: leans towards the fire and breathes (scale only on its own mesh)
	const float breath = std::sin(t * 2.1f);
	glm::mat4 torsoM = root * T(HIPS) * glm::rotate(glm::mat4(1.0f), 0.12f + 0.02f * std::sin(t * 0.5f), X);
	setModel(torsoM * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f + 0.010f * breath, 1.0f + 0.018f * breath, 1.0f + 0.014f * breath)));
	torso.Draw();

	// head: watches the flames, glances around now and then
	glm::mat4 headM = torsoM * T(NECK)
		* glm::rotate(glm::mat4(1.0f), 0.22f * std::sin(t * 0.37f) * std::sin(t * 0.11f), Y)
		* glm::rotate(glm::mat4(1.0f), 0.10f + 0.03f * std::sin(t * 0.9f), X);
	setModel(headM);
	head.Draw();

	// arms: hands held out to the fire, rubbing together in bursts
	const float rubbing = std::max(0.0f, std::sin(t * 0.55f));
	for (int i = 0; i < 2; i++)
	{
		const float s = i ? 1.0f : -1.0f;
		const float phase = i ? 0.0f : 3.14159f;
		glm::mat4 upperM = torsoM * T({ s * SHOULDER.x, SHOULDER.y, SHOULDER.z })
			* glm::rotate(glm::mat4(1.0f), -0.05f + 0.04f * rubbing * std::sin(t * 7.0f + phase), X);
		setModel(upperM);
		upperArm[i].Draw();

		glm::mat4 foreM = upperM * T({ s * ELBOW.x, ELBOW.y, ELBOW.z })
			* glm::rotate(glm::mat4(1.0f), s * 0.16f * rubbing * std::sin(t * 7.0f + phase), Y)
			* glm::rotate(glm::mat4(1.0f), -0.06f * (1.0f - rubbing), X);
		setModel(foreM);
		forearm[i].Draw();
	}
}
