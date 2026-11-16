#include "Menu.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <GL/freeglut.h>

namespace
{
// Text
const char* TITLE = "Snow Globe";
const char* TEAM = "By Sharshiken Ali, Ibrahim Nygyman, Alikhan Nurzhan, Daryn Museyev";
const char* TIP = "Shake hard for a blizzard  -  three shakes wind up the music box";

// Timing
const float LEAVE_SECONDS = 1.6f;      // Start -> sharp picture
const float SWAY_DEGREES = 16.0f;      // camera sway behind the menu
const float SWAY_SPEED = 0.25f;        // radians per second

const glm::vec4 WHITE(1.0f);

float Smooth(float a, float b, float x)
{
	float t = std::clamp((x - a) / (b - a), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

glm::vec4 Fade(glm::vec4 c, float alpha) { c.a *= alpha; return c; }
} // namespace

bool Menu::Init()
{
	ready = ui.Init();
	if (!ready)
	{
		std::fprintf(stderr, "Menu: UI shaders or font failed to load, starting without the menu\n");
		state = State::Playing;
		return false;
	}
	UiFont& font = ui.font;
	title = font.Build(UiFace::Title, TITLE, 0.01f);
	startLabel = font.Build(UiFace::Bold, "Start", 0.08f);
	enterHint = font.Build(UiFace::Body, "or press Enter");
	tip = font.Build(UiFace::Body, TIP);
	byline = font.Build(UiFace::Body, TEAM, 0.01f);
	return true;
}

// state and input

float Menu::BlurAmount() const
{
	if (state == State::Open) return 1.0f;
	if (state == State::Leaving) return 1.0f - Smooth(0.05f, 1.0f, leaving);
	return 0.0f;
}

float Menu::HudAlpha() const
{
	if (state == State::Open) return 0.0f;
	if (state == State::Leaving) return Smooth(0.6f, 1.0f, leaving);
	return 1.0f;
}

float Menu::SwayDegrees() const
{
	float amount = state == State::Open ? 1.0f
		: state == State::Leaving ? 1.0f - Smooth(0.0f, 1.0f, leaving) : 0.0f;
	return SWAY_DEGREES * std::sin(time * SWAY_SPEED) * amount;
}

void Menu::Start()
{
	if (state != State::Open) return;
	state = State::Leaving;
	leaving = 0.0f;
	pressed = false;
	glutSetCursor(GLUT_CURSOR_INHERIT);
}

void Menu::Update(float dt)
{
	time += dt;
	if (state == State::Leaving)
	{
		leaving += dt / LEAVE_SECONDS;
		if (leaving >= 1.0f) { leaving = 1.0f; state = State::Playing; }
	}

	bool over = state == State::Open && OverButton(mouseX, mouseY);
	if (over != hovered)
	{
		hovered = over;
		glutSetCursor(over ? GLUT_CURSOR_INFO : GLUT_CURSOR_INHERIT);
	}
	hover += ((hovered ? 1.0f : 0.0f) - hover) * std::min(1.0f, dt * 12.0f);
}

bool Menu::OverButton(int x, int y) const
{
	return x >= buttonX && x <= buttonX + buttonW && y >= buttonY && y <= buttonY + buttonH;
}

bool Menu::Key(unsigned char key)
{
	if (state != State::Open) return false;
	if (key == 13 || key == ' ') Start();
	return true;                                // nothing else reaches the scene yet
}

bool Menu::Mouse(int button, int buttonState, int x, int y)
{
	mouseX = x;
	mouseY = y;
	if (state != State::Open) return false;
	if (button == GLUT_LEFT_BUTTON)
	{
		if (buttonState == GLUT_DOWN) pressed = OverButton(x, y);
		else if (pressed && OverButton(x, y)) Start();
		else pressed = false;
	}
	return true;
}

void Menu::MouseMove(int x, int y)
{
	mouseX = x;
	mouseY = y;
}

// drawing

void Menu::Draw(int width, int height, bool backgroundBlurred)
{
	if (!ready || state == State::Playing || width <= 0 || height <= 0) return;
	const float w = (float)width, h = (float)height;
	const float s = std::min(w / 1280.0f, h / 720.0f);       // layout is made for 1280 x 720
	const float alpha = 1.0f - Smooth(0.0f, 0.4f, leaving);   // the menu fades first

	ui.Begin(width, height);
	if (!backgroundBlurred)
		ui.Rect(0, 0, w, h, 0, glm::vec4(0.02f, 0.03f, 0.06f, 0.55f * BlurAmount()));

	if (alpha > 0.0f)
	{
		const float cx = 0.5f * w;
		const float rise = -24.0f * s * Smooth(0.0f, 0.5f, leaving);
		auto Y = [&](float y720) { return 0.5f * h + (y720 - 360.0f) * s + rise; };  // centred

		// Title with a soft cold glow
		const float titleSize = 112.0f * s;
		ui.Text(title, cx - 0.5f * title.width * titleSize, Y(318.0f), titleSize,
			Fade(glm::vec4(0.97f, 0.98f, 1.0f, 1.0f), alpha),
			18.0f * s, Fade(glm::vec4(0.55f, 0.75f, 1.0f, 0.38f), alpha));

		// Start button
		const float grow = 1.0f + 0.04f * hover - ((pressed && hovered) ? 0.03f : 0.0f);
		buttonW = 196.0f * s * grow;
		buttonH = 52.0f * s * grow;
		buttonX = cx - 0.5f * buttonW;
		buttonY = Y(404.0f) - 0.5f * buttonH;
		const float glow = 22.0f * s;
		ui.Rect(buttonX - 0.5f * glow, buttonY - 0.5f * glow, buttonW + glow, buttonH + glow,
			0.5f * buttonH + glow, Fade(glm::vec4(0.55f, 0.75f, 1.0f, 0.10f + 0.22f * hover), alpha),
			glm::vec4(0.0f), 0.0f, glow);
		ui.Rect(buttonX, buttonY, buttonW, buttonH, 0.5f * buttonH,
			Fade(glm::vec4(1.0f, 1.0f, 1.0f, 0.10f + 0.14f * hover), alpha),
			Fade(glm::vec4(1.0f, 1.0f, 1.0f, 0.55f + 0.35f * hover), alpha), 1.3f * s);
		ui.TextCentered(startLabel, cx, buttonY + 0.5f * buttonH, 20.0f * s * grow, Fade(WHITE, alpha));
		ui.TextCentered(enterHint, cx, Y(456.0f), 12.5f * s, Fade(glm::vec4(0.85f, 0.9f, 1.0f, 0.55f), alpha));

		ui.TextCentered(tip, cx, h - 64.0f * s + rise, 13.0f * s, Fade(glm::vec4(0.85f, 0.9f, 1.0f, 0.62f), alpha));
		ui.TextCentered(byline, cx, h - 30.0f * s + rise, 14.0f * s, Fade(glm::vec4(0.88f, 0.92f, 1.0f, 0.78f), alpha));
	}
	ui.End();
}
