#include "UIManager.h"
#include "UIButton.h"
#include "Engine.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include <vector>

UIManager::UIManager() :Module()
{
	name = "UIManager";
}

UIManager::~UIManager() {}

bool UIManager::Start()
{
	return true;
}

std::shared_ptr<UIElement> UIManager::CreateUIElement(UIElementType type, int id, const char* text, SDL_Rect bounds, Module* observer, SDL_Rect sliderBounds)
{
	std::shared_ptr<UIElement> uiElement = std::make_shared<UIElement>();

	// L16: TODO 1: Implement CreateUIElement function that instantiates a new UIElement according to the UIElementType and add it to the list of UIElements
	//Call the constructor according to the UIElementType
	switch (type)
	{
	case UIElementType::BUTTON:
		uiElement = std::make_shared<UIButton>(id, bounds, text);
		break;
	}

	//Set the observer
	uiElement->observer = observer;

	// Created GuiControls are add it to the list of controls
	UIElementsList.push_back(uiElement);

	return uiElement;
}

bool UIManager::Update(float dt)
{
	// ── Gamepad navigation (before button updates) ───────────────────────
	UpdateGamepadNavigation(dt);
	ApplyFocusToButtons();

	//List to store entities pending deletion
	std::list<std::shared_ptr<UIElement>> pendingDelete;

	for (const auto& uiElement : UIElementsList)
	{
		//If the entity is marked for deletion, add it to the pendingDelete list
		if (uiElement->pendingToDelete)
		{
			pendingDelete.push_back(uiElement);
		}
		else {
			uiElement->Update(dt);
		}
	}

	//Now iterates over the pendingDelete list and destroys the uiElement
	for (const auto uiElement : pendingDelete)
	{
		uiElement->CleanUp();
		UIElementsList.remove(uiElement);
	}

	return true;
}

bool UIManager::CleanUp()
{
	for (const auto& uiElement : UIElementsList)
	{
		uiElement->CleanUp();
	}
	UIElementsList.clear();
	gamepadFocusIndex = -1;

	return true;
}

// ============================================================================
//  Gamepad Navigation
// ============================================================================

void UIManager::UpdateGamepadNavigation(float dt)
{
	auto& input = *Engine::GetInstance().input;
	if (!input.IsGamepadConnected()) return;

	if (navCooldown > 0.0f) navCooldown -= dt;

	// D-pad or left stick vertical navigation
	bool navUp = input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_UP) == KEY_DOWN ||
	             (input.GetLeftStickY() < -0.5f && navCooldown <= 0.0f);
	bool navDown = input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_DOWN) == KEY_DOWN ||
	               (input.GetLeftStickY() > 0.5f && navCooldown <= 0.0f);

	if (navUp)
	{
		NavigateUp();
		navCooldown = NAV_COOLDOWN_MS;
	}
	else if (navDown)
	{
		NavigateDown();
		navCooldown = NAV_COOLDOWN_MS;
	}

	// Reset cooldown when stick returns to center
	if (std::fabs(input.GetLeftStickY()) < 0.3f &&
	    input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_UP) != KEY_REPEAT &&
	    input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_DOWN) != KEY_REPEAT)
	{
		navCooldown = 0.0f;
	}
}

void UIManager::NavigateDown()
{
	// Collect visible, enabled buttons in order
	std::vector<int> visibleIndices;
	int idx = 0;
	for (const auto& el : UIElementsList)
	{
		if (el->isVisible && el->state != UIElementState::DISABLED &&
		    el->type == UIElementType::BUTTON)
		{
			visibleIndices.push_back(idx);
		}
		idx++;
	}

	if (visibleIndices.empty()) return;

	if (gamepadFocusIndex < 0)
	{
		// No focus yet — pick the first visible button
		gamepadFocusIndex = visibleIndices[0];
		return;
	}

	// Find current position in the visible list
	int currentPos = -1;
	for (int i = 0; i < (int)visibleIndices.size(); i++)
	{
		if (visibleIndices[i] == gamepadFocusIndex) { currentPos = i; break; }
	}

	if (currentPos < 0 || currentPos >= (int)visibleIndices.size() - 1)
		gamepadFocusIndex = visibleIndices[0]; // wrap around
	else
		gamepadFocusIndex = visibleIndices[currentPos + 1];
}

void UIManager::NavigateUp()
{
	// Collect visible, enabled buttons in order
	std::vector<int> visibleIndices;
	int idx = 0;
	for (const auto& el : UIElementsList)
	{
		if (el->isVisible && el->state != UIElementState::DISABLED &&
		    el->type == UIElementType::BUTTON)
		{
			visibleIndices.push_back(idx);
		}
		idx++;
	}

	if (visibleIndices.empty()) return;

	if (gamepadFocusIndex < 0)
	{
		// No focus yet — pick the last visible button
		gamepadFocusIndex = visibleIndices.back();
		return;
	}

	// Find current position in the visible list
	int currentPos = -1;
	for (int i = 0; i < (int)visibleIndices.size(); i++)
	{
		if (visibleIndices[i] == gamepadFocusIndex) { currentPos = i; break; }
	}

	if (currentPos <= 0)
		gamepadFocusIndex = visibleIndices.back(); // wrap around
	else
		gamepadFocusIndex = visibleIndices[currentPos - 1];
}

void UIManager::ApplyFocusToButtons()
{
	int idx = 0;
	for (const auto& el : UIElementsList)
	{
		// Only UIButton has gamepadFocused — dynamic_cast to check
		UIButton* btn = dynamic_cast<UIButton*>(el.get());
		if (btn)
		{
			btn->gamepadFocused = (idx == gamepadFocusIndex && el->isVisible &&
			                       el->state != UIElementState::DISABLED);
		}
		idx++;
	}
}
