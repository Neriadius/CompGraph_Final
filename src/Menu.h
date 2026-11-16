#ifndef MENU_H
#define MENU_H

#include "UiRenderer.h"

// Start menu: the globe, blurred, behind the title "Snow Globe", a Start button (click it or
// press Enter / Space) and the team names. Start fades the menu out and removes the blur
// smoothly (BlurAmount() goes 1 -> 0, BlurPass does the blur). The in-game controls list is
// ControlsOverlay; HudAlpha() fades it in as the blur clears.
class Menu
{
public:
	bool Init();
	void Update(float dt);
	void Draw(int width, int height, bool backgroundBlurred);

	bool InMenu() const { return state == State::Open; }  // game input is blocked
	float BlurAmount() const;                              // 1 in the menu -> 0 after Start
	float HudAlpha() const;                                // 0 in the menu -> 1 in game
	float SwayDegrees() const;                             // slow camera sway behind the menu

	// Input. Each returns true when the menu used the event, so the game must ignore it.
	bool Key(unsigned char key);
	bool Mouse(int button, int buttonState, int x, int y);
	void MouseMove(int x, int y);

private:
	enum class State { Open, Leaving, Playing };

	UiRenderer ui;
	bool ready = false;
	State state = State::Open;
	float leaving = 0.0f;        // 0..1 progress of the Start transition
	float time = 0.0f;
	float hover = 0.0f;          // button hover animation 0..1
	bool hovered = false, pressed = false;
	int mouseX = -1000, mouseY = -1000;
	float buttonX = 0.0f, buttonY = 0.0f, buttonW = 0.0f, buttonH = 0.0f;  // from the last Draw()

	TextMesh title, startLabel, enterHint, tip, byline;

	void Start();
	bool OverButton(int x, int y) const;
};

#endif
