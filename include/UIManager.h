#pragma once

#include "Module.h"
#include "UIElement.h"

#include <list>

class UIManager : public Module
{
public:

	// Constructor
	UIManager();

	// Destructor
	virtual ~UIManager();

	// Called before the first frame
	 bool Start();

	 // Called each loop iteration
	 bool Update(float dt);

	// Called before quitting
	bool CleanUp();

	// Additional methods
	std::shared_ptr<UIElement> CreateUIElement(UIElementType type, int id, const char* text, SDL_Rect bounds, Module* observer, SDL_Rect sliderBounds = { 0,0,0,0 });

	// ── Gamepad navigation ───────────────────────────────────────────────────
	void ResetFocus() { gamepadFocusIndex = -1; }

public:

	std::list<std::shared_ptr<UIElement>> UIElementsList;
	SDL_Texture* texture;

private:
	// ── Gamepad navigation state ─────────────────────────────────────────────
	int   gamepadFocusIndex = -1;   // index into visible buttons (-1 = no focus)
	float navCooldown = 0.0f;       // cooldown timer between D-pad moves (ms)
	static constexpr float NAV_COOLDOWN_MS = 200.0f;

	void UpdateGamepadNavigation(float dt);
	void NavigateUp();
	void NavigateDown();
	void ApplyFocusToButtons();
};
